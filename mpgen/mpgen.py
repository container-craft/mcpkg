#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# mpgen.py — mcpkg MessagePack C code generator

import argparse
import os
import sys
import pathlib
import time
import traceback
from typing import Any, Dict, List, Tuple, Optional

# -------- Debug controls (no CLI changes) --------
DEBUG_LEVEL = 0
try:
    DEBUG_LEVEL = int(os.environ.get("MPGEN_DEBUG", "0"))
except ValueError:
    DEBUG_LEVEL = 0
TRACE_EXC = os.environ.get("MPGEN_TRACE", "0") not in ("", "0", "false", "False", "FALSE")



def dbg(level: int, msg: str) -> None:
    if DEBUG_LEVEL >= level:
        ts = time.strftime("%H:%M:%S")
        print(f"[mpgen][{ts}][D{level}] {msg}", file=sys.stderr)

def dbg_dump(level: int, title: str, obj: Any) -> None:
    if DEBUG_LEVEL >= level:
        import json as _json
        try:
            s = _json.dumps(obj, indent=2, sort_keys=True)
        except Exception:
            s = repr(obj)
        dbg(level, f"{title}:\n{s}")

try:
    import yaml
except Exception as e:
    print(f"[mpgen] ERROR: PyYAML is required: {e}", file=sys.stderr)
    sys.exit(3)

try:
    import json
    import jsonschema  # python3-jsonschema
except Exception as e:
    print(f"[mpgen] ERROR: jsonschema is required: {e}", file=sys.stderr)
    sys.exit(3)

from jinja2 import Environment, FileSystemLoader, StrictUndefined

# ---------------- Utilities ----------------

def ensure_dir(p: str) -> None:
    os.makedirs(p, exist_ok=True)

def write_text_if_changed(path: str, text: str) -> None:
    old = None
    try:
        with open(path, "r", encoding="utf-8") as f:
            old = f.read()
    except FileNotFoundError:
        pass
    if old != text:
        dbg(2, f"write_text_if_changed: writing {path} (len={len(text)})")
        ensure_dir(os.path.dirname(path))
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
    else:
        dbg(3, f"write_text_if_changed: unchanged {path}")

def jinja_env(templates_dir: str) -> Environment:
    dbg(1, f"Initializing Jinja environment: templates_dir={templates_dir}")
    env = Environment(
        loader=FileSystemLoader(templates_dir),
        undefined=StrictUndefined,
        autoescape=False,
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True,
    )
    # Filters
    def c_comment(s: Optional[str]) -> str:
        if not s:
            return ""
        s = str(s).replace("*/", "* /")
        return "/* " + s + " */"
    env.filters["c_comment"] = c_comment

    def c_escape(s: Optional[str]) -> str:
        if s is None:
            return ""
        return (
            str(s)
            .replace("\\", "\\\\")
            .replace("\"", "\\\"")
            .replace("\n", "\\n")
        )
    env.filters["c_escape"] = c_escape

    def header_guard(prefix: str, base: str) -> str:
        g = f"{prefix}_{base}".upper()
        g = "".join(ch if ch.isalnum() else "_" for ch in g)
        return g
    env.filters["header_guard"] = header_guard
    env.globals["header_guard"] = header_guard  # <-- add this line


    def include_basename(path: str) -> str:
        return os.path.basename(path)
    env.filters["include_basename"] = include_basename

    def ref_sym_from_include(path: str) -> str:
        base = os.path.basename(path)
        if base.endswith(".h"):
            base = base[:-2]
        if base.startswith("mcpkg_mp_"):
            base = base[len("mcpkg_mp_"):]
        return base
    env.filters["ref_sym_from_include"] = ref_sym_from_include

    return env

def load_yaml(path: str) -> Dict[str, Any]:
    dbg(1, f"Loading YAML: {path}")
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    if not isinstance(data, dict):
        raise ValueError("top-level YAML must be a mapping")
    dbg_dump(3, f"YAML loaded ({path})", data)
    return data

def load_json_schema_file(path: str) -> Dict[str, Any]:
    dbg(1, f"Loading JSON Schema: {path}")
    try:
        with open(path, "r", encoding="utf-8") as f:
            doc = json.load(f)
            dbg(1, f"Loaded schema $id={doc.get('$id')}, $schema={doc.get('$schema')}")
            return doc
    except FileNotFoundError:
        raise RuntimeError(f"[mpgen] schema JSON not found: {path}")
    except Exception as e:
        raise RuntimeError(f"[mpgen] failed to load JSON schema {path}: {e}")

def validate_schema_dict(data: Dict[str, Any], schema_doc: Dict[str, Any], path: str) -> None:
    dbg(1, f"Validating against schema: {path}")
    try:
        jsonschema.validate(instance=data, schema=schema_doc)
        dbg(1, f"Schema OK: {path}")
    except jsonschema.exceptions.ValidationError as e:
        loc = " → ".join(str(x) for x in e.relative_path) if e.relative_path else ""
        schloc = " → ".join(str(x) for x in e.schema_path) if e.schema_path else ""
        msg = f"{path}: {e.message}"
        if loc:
            msg += f" (at {loc})"
        if schloc:
            msg += f" [schema path: {schloc}]"
        if TRACE_EXC:
            print("[mpgen] TRACE: validation error traceback:", file=sys.stderr)
            traceback.print_exc()
        raise ValueError(msg) from e

def parse_bin_decl(t: str) -> Optional[int]:
    if t.startswith("bin[") and t.endswith("]"):
        n = t[4:-1]
        try:
            v = int(n, 10)
            return v if v > 0 else None
        except ValueError:
            return None
    return None

def parse_list_bin_decl(t: str) -> Optional[int]:
    if t.startswith("list<bin[") and t.endswith("]>"):
        n = t[len("list<bin["):-2]
        try:
            v = int(n, 10)
            return v if v > 0 else None
        except ValueError:
            return None
    return None

def normalize_fields(fields_node: Any) -> List[Dict[str, Any]]:
    """Return list of normalized field dicts with: key,name,type,kind,size?,
       ctype?,ref_include?,ref_sym?,required?,comment?"""
    dbg(2, "Normalizing fields…")
    items: List[Tuple[int, Dict[str, Any]]] = []
    if isinstance(fields_node, dict):
        for k_str, f in fields_node.items():
            if not isinstance(f, dict):
                raise ValueError("each field entry must be a mapping")
            try:
                k = int(k_str, 10)
            except Exception:
                raise ValueError(f"field key '{k_str}' is not an integer")
            f = dict(f)
            f["key"] = k
            items.append((k, f))
    elif isinstance(fields_node, list):
        for f in fields_node:
            if not isinstance(f, dict):
                raise ValueError("each field entry must be a mapping")
            if "key" not in f:
                raise ValueError("array form requires 'key' in each field")
            items.append((int(f["key"]), dict(f)))
    else:
        raise ValueError("'fields' must be a map or an array")

    items.sort(key=lambda kv: kv[0])
    dbg_dump(3, "Fields (raw, sorted)", items)

    # Enforce unique keys (and optionally forbid key 0 if desired)
    seen = set()
    for k, _ in items:
        if k in seen:
            raise ValueError(f"duplicate field key: {k}")
        seen.add(k)

    out: List[Dict[str, Any]] = []
    for _, f in items:
        t = f.get("type")
        if not isinstance(t, str):
            raise ValueError("field 'type' must be a string")
        name = f.get("name")
        if not isinstance(name, str):
            raise ValueError("field 'name' must be a string")

        kind = None
        size = None
        ctype = f.get("ctype")
        if isinstance(ctype, str) and ctype.startswith("struct "):
            dbg(2, f"{name}: stripping leading 'struct ' from ctype={ctype!r}")
            ctype = ctype.split(" ", 1)[1]
        ref_include = f.get("ref_include")
        ref_sym = f.get("ref_sym")

        if t in ("str", "i32", "u32", "i64", "u64"):
            kind = "SCALAR"
        elif t == "bin" or t.startswith("bin["):
            kind = "BIN"
            if t == "bin":
                size = f.get("size")
                if not isinstance(size, int) or size <= 0:
                    raise ValueError(f"{name}: 'bin' requires positive 'size'")
                dbg(2, f"{name}: BIN(size={size}) via 'bin' + size prop")
            else:
                size = parse_bin_decl(t)
                if not size:
                    raise ValueError(f"{name}: invalid {t}")
                dbg(2, f"{name}: BIN(size={size}) via 'bin[N]'")
        elif t == "struct":
            kind = "STRUCT"
            if not ctype or not ref_include:
                raise ValueError(f"{name}: 'struct' requires 'ctype' and 'ref_include'")
        elif t.startswith("list<"):
            if t == "list<str>":
                kind = "LIST_SCALAR"
            elif t == "list<bin>":
                kind = "LIST_BIN"
                size = f.get("size")
                if not isinstance(size, int) or size <= 0:
                    raise ValueError(f"{name}: 'list<bin>' requires positive 'size'")
                dbg(2, f"{name}: LIST_BIN(size={size}) via 'list<bin>' + size prop")
            elif t.startswith("list<bin["):
                kind = "LIST_BIN"
                size = parse_list_bin_decl(t)
                if not size:
                    raise ValueError(f"{name}: invalid {t}")
                dbg(2, f"{name}: LIST_BIN(size={size}) via 'list<bin[N]>'")
            elif t == "list<struct>":
                kind = "LIST_STRUCT"
                if not ctype or not ref_include:
                    raise ValueError(f"{name}: 'list<struct>' requires 'ctype' and 'ref_include'")
            else:
                raise ValueError(f"{name}: unsupported list type '{t}'")
        else:
            raise ValueError(f"{name}: unsupported type '{t}'")

        nf: Dict[str, Any] = {
            "key": int(f["key"]),
            "name": name,
            "type": t,
            "kind": kind,
            "required": bool(f.get("required", False)),
            "comment": f.get("comment", None),
        }
        if size is not None:
            nf["size"] = int(size)
        if ctype:
            nf["ctype"] = str(ctype)
        if ref_include:
            nf["ref_include"] = str(ref_include)
        if ref_sym:
            nf["ref_sym"] = str(ref_sym)

        # convenience alias for templates that expect 'ftype'
        nf["ftype"] = nf["type"]

        out.append(nf)

    # Derive ref_sym where needed (STRUCT/LIST_STRUCT)
    for f in out:
        if f["kind"] in ("STRUCT", "LIST_STRUCT"):
            if "ref_include" not in f:
                continue
            if "ref_sym" not in f:
                inc = f["ref_include"]
                base = os.path.basename(inc)
                if base.endswith(".h"):
                    base = base[:-2]
                if base.startswith("mcpkg_mp_"):
                    base = base[len("mcpkg_mp_"):]
                f["ref_sym"] = base
                dbg(2, f"{f['name']}: derived ref_sym={f['ref_sym']} from ref_include={inc}")

    if DEBUG_LEVEL >= 2:
        # Only show key details to keep logs readable
        slim = [
            {k: fld[k] for k in ("key", "name", "type", "kind", "size", "ctype", "ref_include", "ref_sym", "required") if k in fld}
            for fld in out
        ]
        dbg_dump(2, "Fields (normalized summary)", slim)

    return out

def load_and_normalize_schema(path: str, schema_doc: Dict[str, Any]) -> Dict[str, Any]:
    dbg(1, f"Processing schema file: {path}")
    data = load_yaml(path)
    validate_schema_dict(data, schema_doc, path)

    fields = normalize_fields(data["fields"])

    include_prefix = data["include_prefix"]
    out_base = data["out_base"]
    hdr_name = f"{include_prefix}/mcpkg_mp_{out_base}.h"
    src_name = f"{include_prefix}/mcpkg_mp_{out_base}.c"

    normalized = {
        "tag": data["tag"],
        "version": int(data["version"]),
        "unit": data["unit"],
        "base": data["base"],
        "c_struct": data["c_struct"],
        "include_prefix": include_prefix,
        "out_base": out_base,
        "fields": fields,
        "hdr_name": hdr_name,
        "src_name": src_name,
        "schema_path": path,
    }
    dbg_dump(2, "Normalized schema (summary)", {
        "tag": normalized["tag"],
        "version": normalized["version"],
        "unit": normalized["unit"],
        "base": normalized["base"],
        "c_struct": normalized["c_struct"],
        "include_prefix": normalized["include_prefix"],
        "out_base": normalized["out_base"],
        "hdr_name": normalized["hdr_name"],
        "src_name": normalized["src_name"],
        "fields_count": len(fields),
    })
    return normalized

def render_schema(env: Environment, sch: Dict[str, Any], out_dir: str) -> List[str]:
    dbg(1, f"Rendering templates for {sch.get('unit')}.{sch.get('base')} -> {sch.get('include_prefix')}/mcpkg_mp_{sch.get('out_base')}.{{h,c}}")
    hdr_tpl = env.get_template("c_header.jinja")
    src_tpl = env.get_template("c_source.jinja")

    hdr_text = hdr_tpl.render(sch=sch)
    src_text = src_tpl.render(sch=sch)

    hdr_out = os.path.join(out_dir, sch["hdr_name"])
    src_out = os.path.join(out_dir, sch["src_name"])

    ensure_dir(os.path.dirname(hdr_out))
    ensure_dir(os.path.dirname(src_out))

    write_text_if_changed(hdr_out, hdr_text)
    write_text_if_changed(src_out, src_text)

    dbg(1, f"Wrote: {hdr_out}")
    dbg(1, f"Wrote: {src_out}")
    return [hdr_out, src_out]

# ---------------- Schema file discovery ----------------

def gather_schema_files(paths: List[str]) -> List[str]:
    dbg(1, f"Gathering schema files from: {paths}")
    files: List[str] = []
    for p in paths:
        pth = pathlib.Path(p)
        if pth.is_dir():
            for ext in ("*.yml", "*.yaml"):
                for sp in pth.rglob(ext):
                    if any(part.startswith(".") for part in sp.parts):
                        continue
                    files.append(str(sp))
        elif pth.is_file():
            if pth.suffix.lower() in (".yml", ".yaml"):
                files.append(str(pth))
        else:
            print(f"[mpgen] WARNING: path not found: {p}", file=sys.stderr)
    files = sorted(set(os.path.abspath(f) for f in files))
    dbg_dump(1, "Discovered schema files", files)
    return files

# ---------------- Main ----------------

def main() -> None:
    script_dir = os.path.abspath(os.path.dirname(__file__))
    default_templates = os.path.join(script_dir, "templates")
    schema_js = os.path.join(script_dir, "schemas/mpgen_schema.json")

    ap = argparse.ArgumentParser(description="mcpkg mp code generator")
    ap.add_argument(
        "--schemas",
        nargs="+",
        required=True,
        help="space-separated list of YAML schema files Example schema/a.yaml schema/b.yaml",
    )
    ap.add_argument(
        "--templates",
        default=default_templates,
        help=f"templates directory (default: {default_templates})",
    )
    ap.add_argument("--out", required=True, help="output root directory")
    ap.add_argument("--fail-fast", action="store_true", help="stop on first schema error")
    args = ap.parse_args()

    # Load external JSON Schema
    schema_doc = load_json_schema_file(schema_js)

    env = jinja_env(args.templates)
    schema_paths = [os.path.abspath(s) for s in args.schemas]
    dbg_dump(1, "Schema files (explicit)", schema_paths)

    if not schema_paths:
        print(f"[mpgen] no schemas found under {args.schemas}", file=sys.stderr)
        sys.exit(1)

    wrote: List[str] = []
    errors: List[str] = []

    print(f"[mpgen] generating from {len(schema_paths)} schema(s)")
    schema_id = schema_doc.get("$id", "<no $id>")
    print(f"[mpgen] using schema: {schema_js} ({schema_id})")

    for spath in schema_paths:
        try:
            sch = load_and_normalize_schema(spath, schema_doc)
            wrote.extend(render_schema(env, sch, args.out))
            print(f"[mpgen] ok: {os.path.relpath(spath)} -> {sch.get('include_prefix')}/mcpkg_mp_{sch.get('out_base')}.{{h,c}}")
        except Exception as e:
            msg = f"{spath}: {e}"
            errors.append(msg)
            print(f"[mpgen] ERROR: {msg}", file=sys.stderr)
            if TRACE_EXC:
                traceback.print_exc()
            if args.fail_fast:
                break

    if errors:
        print(f"[mpgen] {len(errors)} error(s)", file=sys.stderr)
        sys.exit(2)

    # Touch a stamp to help CMake dependency chains
    stamp = os.path.join(args.out, ".stamp")
    ensure_dir(args.out)
    write_text_if_changed(stamp, "ok\n")

if __name__ == "__main__":
    main()

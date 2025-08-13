#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# mpgen.py — generate C structs + MessagePack pack/unpack/debug code
#
# Usage example:
#   python3 mpgen.py \
#       --schemas mpgen/schemas mpgen/schemas/sig.yaml \
#       --out build/mpgen/gen
#
# Notes:
# - --schemas accepts files and/or directories. Directories are recursively
#   scanned for *.y*ml.
# - If a schema omits 'unit' or 'base', they are derived from the path:
#     - <...>/schemas/<unit>/<name>.yaml         -> base=<name>
#     - <...>/schemas/<name>.yaml                -> unit=<name>, base=main
#     - <...>/schemas/<unit>_<base>.yaml         -> split on '_' (or '-')
# - Produces mcpkg_mp_<unit>_<base>.{h,c} in --out.

import argparse
import os
import pathlib
import sys
import re
from typing import Any, Dict, List, Tuple

try:
    import yaml
except Exception as e:  # pragma: no cover
    print("[mpgen] missing dependency: PyYAML", file=sys.stderr)
    raise

from jinja2 import Environment, FileSystemLoader

# ---------------- Filesystem helpers ----------------

def ensure_dir(p: str) -> None:
    os.makedirs(p, exist_ok=True)

def write_text_if_changed(path: str, txt: str) -> None:
    ensure_dir(os.path.dirname(path) or ".")
    try:
        with open(path, "r", encoding="utf-8") as f:
            cur = f.read()
        if cur == txt:
            return
    except FileNotFoundError:
        pass
    with open(path, "w", encoding="utf-8") as f:
        f.write(txt)

def load_text(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

# ---------------- Schema load/normalize ----------------

def load_yaml(path: str) -> Dict[str, Any]:
    return yaml.safe_load(load_text(path)) or {}

def camelize(s: str) -> str:
    return "".join(part.capitalize() for part in re.split(r"[_\-\s]+", s) if part)

def _stem(path: str) -> str:
    return os.path.splitext(os.path.basename(path))[0]

def _parent_name(path: str) -> str:
    return os.path.basename(os.path.dirname(path))

def _under_schemas_parent(path: str) -> str:
    # returns dir name immediately under ".../schemas/" if present, else parent
    parts = pathlib.Path(path).parts
    try:
        idx = len(parts) - 1 - list(reversed(parts)).index("schemas")
        # idx points to "schemas"
        if idx < len(parts) - 1:
            return parts[idx + 1]
    except ValueError:
        pass
    return _parent_name(path)

def _derive_unit_base_from_path(schema_path: str) -> Tuple[str, str]:
    """
    Best-effort derivation:
      1) If under .../schemas/<unit>/<file>.yaml -> unit=<unit>, base=<file>
      2) If filename has "<unit>_<base>.yaml" or "<unit>-<base>.yaml"
      3) Fallback: unit=<file>, base=main
    """
    stem = _stem(schema_path)
    parent = _under_schemas_parent(schema_path)

    # Case 1: <schemas>/<unit>/<file>.yaml
    # If parent != "schemas" and not the filesystem root sentinel
    if parent and parent != "schemas":
        return parent, stem

    # Case 2: split on '_' or '-'
    m = re.match(r"^([A-Za-z0-9]+)[_\-]([A-Za-z0-9].*)$", stem)
    if m:
        return m.group(1), m.group(2)

    # Case 3: last resort
    return stem, "main"

def derive_names(schema_path: str, sch: Dict[str, Any]) -> Dict[str, Any]:
    """
    Fill computed fields:
      - unit/base (with fallback from path if missing)
      - out_base, c_struct, macro names
      - include_prefix default
    """
    unit = (sch.get("unit") or "").strip()
    base = (sch.get("base") or "").strip()

    if not unit or not base:
        u2, b2 = _derive_unit_base_from_path(schema_path)
        unit = unit or u2
        base = base or b2

    if not unit or not base:
        raise ValueError("schema requires 'unit' and 'base' keys (could not derive from path)")

    out_base = f"{unit}_{base}"
    c_struct = "McPkg" + camelize(out_base)

    macro_prefix_up = unit.upper()
    macro_base_up = base.upper()

    tag = str(sch.get("tag", f"mcpkg.{unit}.{base}.v1"))
    version = int(sch.get("version", 1))

    include_prefix = sch.get("include_prefix") or "."

    return {
        "path": schema_path,
        "unit": unit,
        "base": base,
        "out_base": out_base,
        "c_struct": c_struct,
        "macro_prefix_up": macro_prefix_up,
        "macro_base_up": macro_base_up,
        "version": version,
        "tag": tag,
        "include_prefix": include_prefix,
    }

_FIELD_RE_BIN = re.compile(r"^bin\[(\d+)\]$")
_FIELD_RE_STRUCT = re.compile(r"^struct<\s*([A-Za-z_][A-Za-z0-9_]*)\s*>$")
_FIELD_RE_LIST = re.compile(r"^list<\s*(.+?)\s*>$")

def _derive_ref_unit_from_include(ref_include: str) -> str:
    # "path/to/mcpkg_mp_foo_bar.h" -> "foo_bar"
    b = os.path.basename(ref_include)
    if b.endswith(".h"):
        b = b[:-2]
    if b.startswith("mcpkg_mp_"):
        b = b[len("mcpkg_mp_"):]
    return b

def normalize_fields(sch: Dict[str, Any]) -> Tuple[List[Dict[str, Any]], List[str]]:
    """
    Normalize schema['fields'] into list of dicts with:
      name, key, required,
      kind: 'scalar' | 'list_str' | 'struct_bin' | 'list_struct_bin'
      ftype (for scalar): 'str'|'i32'|'u32'|'i64'|'bin'
      size (for bin) ; ref_struct/ref_include/ref_unit for nested
    """
    fields_in = sch.get("fields", [])
    if not isinstance(fields_in, list) or not fields_in:
        raise ValueError("schema must have non-empty 'fields' array")

    out: List[Dict[str, Any]] = []
    src_includes: List[str] = []

    for f in fields_in:
        name = f.get("name")
        key = f.get("key")
        ftype = f.get("type")
        required = bool(f.get("required", False))
        if name is None or key is None or ftype is None:
            raise ValueError("each field needs name, key, type")

        nf: Dict[str, Any] = {
            "name": name,
            "key": int(key),
            "required": required,
        }

        # list<T>
        m_list = _FIELD_RE_LIST.match(str(ftype))
        if m_list:
            inner = m_list.group(1).strip()
            if inner == "str":
                nf["kind"] = "list_str"
                out.append(nf)
                continue

            m_struct = _FIELD_RE_STRUCT.match(inner)
            ref_struct = None
            if m_struct:
                ref_struct = m_struct.group(1)
            else:
                if re.match(r"^[A-Za-z_]\w*$", inner):
                    ref_struct = inner
            if not ref_struct:
                raise ValueError(f"unsupported list<> inner type: {inner}")

            nf["kind"] = "list_struct_bin"
            nf["ref_struct"] = ref_struct

            ref_include = f.get("ref_include")
            ref_unit = f.get("ref_unit")

            if not ref_unit:
                if ref_include:
                    ref_unit = _derive_ref_unit_from_include(ref_include)
                else:
                    raise ValueError(f"{name}: list_struct_bin requires ref_include or ref_unit")
            nf["ref_unit"] = ref_unit

            if ref_include:
                nf["ref_include"] = ref_include
                if ref_include not in src_includes:
                    src_includes.append(ref_include)

            out.append(nf)
            continue

        # struct<...>
        m_struct = _FIELD_RE_STRUCT.match(str(ftype))
        if m_struct:
            nf["kind"] = "struct_bin"
            nf["ref_struct"] = m_struct.group(1)

            ref_include = f.get("ref_include")
            ref_unit = f.get("ref_unit")
            if not ref_unit:
                if ref_include:
                    ref_unit = _derive_ref_unit_from_include(ref_include)
                else:
                    raise ValueError(f"{name}: struct_bin requires ref_include or ref_unit")
            nf["ref_unit"] = ref_unit

            if ref_include:
                nf["ref_include"] = ref_include
                if ref_include not in src_includes:
                    src_includes.append(ref_include)
            out.append(nf)
            continue

        # scalar
        nf["kind"] = "scalar"
        st = str(ftype)
        if st in ("str", "i32", "u32", "i64"):
            nf["ftype"] = st
        else:
            m_bin = _FIELD_RE_BIN.match(st)
            if m_bin:
                nf["ftype"] = "bin"
                nf["size"] = int(m_bin.group(1))
            else:
                raise ValueError(f"unsupported scalar type: {ftype}")
        out.append(nf)

    out.sort(key=lambda d: d["key"])
    return out, src_includes

# ---------------- Jinja environment ----------------

def jinja_env(templates_path: str) -> Environment:
    loader = FileSystemLoader(templates_path)
    env = Environment(loader=loader, trim_blocks=True, lstrip_blocks=True)

    def c_comment(s: str) -> str:
        if s is None:
            return ""
        return str(s).replace("/*", "/ *").replace("*/", "* /")

    def field_macro(f: Dict[str, Any]) -> str:
        k = f.get("kind")
        if k == "scalar":
            t = f.get("ftype")
            if t == "bin":
                return f"bin[{f.get('size', 0)}]"
            return t
        if k == "list_str":
            return "list<str>"
        if k == "struct_bin":
            return f"struct<{f.get('ref_struct','?')}>"
        if k == "list_struct_bin":
            return f"list<struct<{f.get('ref_struct','?')}> >"
        return "unknown"

    env.filters["c_comment"] = c_comment
    env.filters["field_macro"] = field_macro
    return env

# ---------------- Rendering ----------------

def render_schema(env: Environment, sch: Dict[str, Any], out_root: str) -> List[str]:
    hdr_tpl = env.get_template("c_header.jinja")
    src_tpl = env.get_template("c_source.jinja")

    ctx: Dict[str, Any] = {}
    ctx.update(derive_names(sch["__path__"], sch))
    fields, src_includes = normalize_fields(sch)
    ctx["fields"] = fields
    ctx["src_includes"] = src_includes

    out_base = ctx["out_base"]
    hdr_name = f"mcpkg_mp_{out_base}.h"
    src_name = f"mcpkg_mp_{out_base}.c"

    ensure_dir(out_root)
    hdr_path = os.path.join(out_root, hdr_name)
    src_path = os.path.join(out_root, src_name)

    hdr_txt = hdr_tpl.render(schema=ctx, **ctx)
    src_txt = src_tpl.render(schema=ctx, **ctx)

    write_text_if_changed(hdr_path, hdr_txt)
    write_text_if_changed(src_path, src_txt)

    return [hdr_path, src_path]

# ---------------- Discovery ----------------

def find_schema_files(root: str) -> List[str]:
    files: List[str] = []
    for p in pathlib.Path(root).rglob("*.y*ml"):
        if any(part.startswith(".") for part in p.parts):
            continue
        files.append(str(p))
    files.sort()
    return files

# ---------------- Main ----------------

def main() -> None:
    script_dir = os.path.abspath(os.path.dirname(__file__))
    default_templates = os.path.join(script_dir, "templates")

    ap = argparse.ArgumentParser(description="mcpkg mp code generator")
    ap.add_argument(
        "--schemas",
        nargs="+",
        required=True,
        help="space-separated list of YAML schema files and/or directories",
    )
    ap.add_argument(
        "--templates",
        default=default_templates,
        help=f"templates directory (default: {default_templates})",
    )
    ap.add_argument("--out", required=True, help="output root directory")
    ap.add_argument("--fail-fast", action="store_true", help="stop on first schema error")
    args = ap.parse_args()

    # Collect schema files
    schema_paths: List[str] = []
    for item in args.schemas:
        if os.path.isdir(item):
            schema_paths.extend(find_schema_files(item))
        elif os.path.isfile(item):
            schema_paths.append(item)
        else:
            print(f"[mpgen] warning: not found: {item}", file=sys.stderr)

    schema_paths = sorted(set(schema_paths))
    if not schema_paths:
        print(f"[mpgen] no schemas found among: {args.schemas}", file=sys.stderr)
        sys.exit(1)

    env = jinja_env(args.templates)

    wrote: List[str] = []
    errors: List[str] = []

    print(f"[mpgen] generating from {len(schema_paths)} schema(s)")
    for spath in schema_paths:
        try:
            sch = load_yaml(spath)
            if not isinstance(sch, dict):
                raise ValueError("schema is not a mapping")
            sch["__path__"] = spath
            wrote.extend(render_schema(env, sch, args.out))
            meta = derive_names(spath, sch)
            print(f"[mpgen] ok: {os.path.relpath(spath)} -> "
                  f"{meta.get('include_prefix')}/mcpkg_mp_{meta.get('out_base')}.{{h,c}}")
        except Exception as e:
            msg = f"{spath}: {e}"
            errors.append(msg)
            print(f"[mpgen] ERROR: {msg}", file=sys.stderr)
            if args.fail_fast:
                break

    if errors:
        print(f"[mpgen] {len(errors)} error(s)", file=sys.stderr)
        sys.exit(2)

    ensure_dir(args.out)
    write_text_if_changed(os.path.join(args.out, ".stamp"), "ok\n")

if __name__ == "__main__":
    main()

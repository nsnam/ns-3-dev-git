#!/bin/bash
# dia(1) wrapper: decompress gzip-compressed .dia inputs before invoking dia.
#
# dia saves .dia files gzip-compressed, but recent libxml2 (2.14+) no longer
# transparently decompresses XML input, so dia 0.97.x fails to load its own
# files ("Start tag expected, '<' not found").  This wrapper feeds dia a
# decompressed copy of any gzip-compressed .dia argument; it is a no-op for
# already-uncompressed inputs and for every other argument, so it works on
# every dia/libxml2 combination.
#
# Set DIA_BIN to override the real dia binary (default: dia, found via PATH).

DIA_BIN="${DIA_BIN:-dia}"

workdir=
cleanup() {
    [ -n "$workdir" ] && rm -rf "$workdir"
}
trap cleanup EXIT INT TERM

# Create the per-invocation temp directory on first use (per-invocation so
# concurrent make -j jobs never collide).  It is created under the doc build
# dir ($BUILDDIR, exported by defines.mk; defaults to ./build), so leftovers
# from a hard kill are localized and swept by `make clean`
# (rm -rf $(BUILDDIR)/*).  Figures are built before sphinx creates $BUILDDIR,
# so mkdir -p it first.
check_workdir() {
    if [ -z "$workdir" ]; then
        if [ -z "${BUILDDIR:-}" ]; then
            echo "dia-wrapper.sh: warning: BUILDDIR unset, using ./build" \
                 "for temp files (run via make, or set BUILDDIR)" >&2
        fi
        tmproot="${BUILDDIR:-build}"
        mkdir -p "$tmproot" || exit 1
        workdir=$(mktemp -d "$tmproot/ns3-dia.XXXXXX") || exit 1
    fi
}

# Rebuild the argument list, replacing each gzip-compressed .dia file with a
# decompressed temporary copy.  The copy keeps the original basename, so dia's
# diagnostics still name the right file.
new_args=()
for arg in "$@"; do
    case "$arg" in
        *.dia)
            if [ -f "$arg" ] && gzip -t "$arg" 2>/dev/null; then
                check_workdir
                copy="$workdir/$(basename "$arg")"
                gzip -dc "$arg" > "$copy" || exit 1
                arg="$copy"
            fi
            ;;
    esac
    new_args+=("$arg")
done

"$DIA_BIN" "${new_args[@]}"

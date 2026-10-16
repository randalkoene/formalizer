#!/bin/bash
#
# fzclaude-install.sh - Install the live copies of fzclaude, its guard and its web interface.
#
# Randal A. Koene, 20261014
#
# Claude works on fzclaude in the repository, but the copies that actually run
# (the scope filter, the guard hook, the web server) are installed in
# ~/.formalizer/fzclaude/bin, which the guard protects from Claude. So a change
# Claude makes in the repository takes effect only when you install it.
#
# Shows the differences between the installed and the repository versions,
# asks before installing, links fzclaude, fzclaude-guard and
# fzclaude-web-start in ~/.formalizer/bin to the installed copies, and appends
# a line to ~/.formalizer/fzclaude/install.log.
#
#   make fzclaude-install          # from the Formalizer repository root
#   fzclaude-install.sh -y         # install without asking

set -e

if [ -n "$CLAUDECODE" ]; then
    echo "fzclaude-install must be run by you, not by Claude."
    exit 1
fi

SRC="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
STATE="$HOME/.formalizer/fzclaude"
DEST="$STATE/bin"
FZBIN="$HOME/.formalizer/bin"
PROGRAMS="fzclaude.py fzclaude-guard.py fzclaude-web.py fzclaude-web-start.py"
WEBFILES="index.html app.css app.js"

assume_yes=false
[ "$1" = "-y" ] && assume_yes=true

echo "Installing fzclaude from $SRC"
echo "                     into $DEST"
echo

changes=0
for f in $PROGRAMS; do
    if [ ! -f "$DEST/$f" ]; then
        echo "new: $f"; changes=1
    elif ! cmp -s "$SRC/$f" "$DEST/$f"; then
        diff -u "$DEST/$f" "$SRC/$f" --label "installed/$f" --label "repository/$f" || true
        changes=1
    fi
done
for f in $WEBFILES; do
    if [ ! -f "$DEST/web/$f" ]; then
        echo "new: web/$f"; changes=1
    elif ! cmp -s "$SRC/web/$f" "$DEST/web/$f"; then
        diff -u "$DEST/web/$f" "$SRC/web/$f" --label "installed/web/$f" --label "repository/web/$f" || true
        changes=1
    fi
done

if [ $changes -eq 0 ]; then
    echo "The installed copies are identical to the repository versions."
else
    echo
    if ! $assume_yes; then
        read -r -p "Install these changes? [y/N] " answer
        if [ "$answer" != "y" ] && [ "$answer" != "Y" ]; then
            echo "Nothing installed."
            exit 0
        fi
    fi
    mkdir -p -m 700 "$STATE"
    mkdir -p "$DEST/web"
    for f in $PROGRAMS; do install -m 755 "$SRC/$f" "$DEST/$f"; done
    for f in $WEBFILES; do install -m 644 "$SRC/web/$f" "$DEST/web/$f"; done
    rev="$(git -C "$SRC" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    dirty="$(git -C "$SRC" status --porcelain -- . 2>/dev/null | head -1)"
    echo "$(date '+%Y-%m-%d %H:%M:%S') installed from $rev${dirty:+ (with uncommitted changes)}" >> "$STATE/install.log"
    echo "Installed."
fi

# The commands in ~/.formalizer/bin always point at the installed copies.
mkdir -p "$FZBIN"
for f in fzclaude fzclaude-guard fzclaude-web-start; do
    ln -sf "$DEST/$f.py" "$FZBIN/$f.py"
    ln -sf "$f.py" "$FZBIN/$f"
done
echo "Linked fzclaude, fzclaude-guard and fzclaude-web-start in $FZBIN to the installed copies."

if (exec 3<>/dev/tcp/127.0.0.1/8712) 2>/dev/null; then
    echo "Note: fzclaude-web is running. Stop it (the ⏻ button) and start it again from the dashboard to use the new version."
fi

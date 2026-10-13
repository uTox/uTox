# uTox includes <OpenAL/...>; OpenAL Soft installs headers under AL/.
#
# Optional env:
#   OPENAL_INCLUDE_DIR — directory that contains AL/ (default: $CACHE_DIR/usr/include)

OPENAL_INCLUDE_DIR="${OPENAL_INCLUDE_DIR:-$CACHE_DIR/usr/include}"

if [ -d "$OPENAL_INCLUDE_DIR/AL" ]; then
  case "$(uname -s 2>/dev/null)" in
    CYGWIN*|MSYS*|MINGW*)
      # Native MinGW tools do not follow Cygwin symlinks — copy instead.
      if [ -L "$OPENAL_INCLUDE_DIR/OpenAL" ]; then
        rm -f "$OPENAL_INCLUDE_DIR/OpenAL"
      fi
      if [ ! -e "$OPENAL_INCLUDE_DIR/OpenAL" ]; then
        cp -a "$OPENAL_INCLUDE_DIR/AL" "$OPENAL_INCLUDE_DIR/OpenAL"
      fi
      ;;
    *)
      if [ ! -e "$OPENAL_INCLUDE_DIR/OpenAL" ]; then
        ln -sf AL "$OPENAL_INCLUDE_DIR/OpenAL"
      fi
      ;;
  esac
fi

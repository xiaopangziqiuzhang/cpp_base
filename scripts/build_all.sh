#!/usr/bin/env bash
# 全量编译并运行 demos/ 下的所有 .cpp，输出结果写到 build/out/
#
# 用法:
#   ./scripts/build_all.sh              # 增量：跳过已成功且源文件未更新的
#   ./scripts/build_all.sh --force      # 全量重编
#   ./scripts/build_all.sh --jobs 32    # 指定并行进程数
#   ./scripts/build_all.sh part1_basics # 只编译某个子目录
#
# 设计要点：
#   - 进程级并行（xargs -P），每个 demo 独立编译+运行，互不影响
#   - resume：成功记录落盘到 build/state/<hash>.ok，再次运行直接跳过
#   - 每个 demo 的编译/运行日志单独落盘，便于定位

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO_DIR="$ROOT/demos"
BUILD_DIR="$ROOT/build"
BIN_DIR="$BUILD_DIR/bin"
OUT_DIR="$BUILD_DIR/out"
LOG_DIR="$BUILD_DIR/log"
STATE_DIR="$BUILD_DIR/state"

JOBS="$(nproc)"
FORCE=0
FILTER=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --force) FORCE=1; shift ;;
    --jobs)  JOBS="$2"; shift 2 ;;
    -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
    *) FILTER="$1"; shift ;;
  esac
done

mkdir -p "$BIN_DIR" "$OUT_DIR" "$LOG_DIR" "$STATE_DIR"

# 单个 demo 的编译+运行逻辑，供 xargs 调用
if [[ "${1:-}" == "--worker" ]]; then :; fi

export ROOT DEMO_DIR BIN_DIR OUT_DIR LOG_DIR STATE_DIR FORCE

run_one() {
  local src="$1"
  local rel="${src#"$DEMO_DIR"/}"
  local name="${rel%.cpp}"
  local flat="${name//\//__}"

  # demo 首行形如:  // std=c++17 flags=-pthread
  local hdr std flags
  hdr="$(head -1 "$src")"
  std="$(sed -n 's/.*std=\(c++[0-9a-z]*\).*/\1/p' <<<"$hdr")"
  flags="$(sed -n 's/.*flags=\(.*\)$/\1/p' <<<"$hdr")"
  [[ -z "$std" ]] && std="c++17"

  # resume: 源文件内容哈希一致则跳过
  local sig ok_file
  sig="$(cat "$src" <(echo "$std$flags") | md5sum | cut -c1-32)"
  ok_file="$STATE_DIR/${flat}.${sig}.ok"
  if [[ $FORCE -eq 0 && -f "$ok_file" ]]; then
    echo "SKIP  $rel"
    return 0
  fi
  rm -f "$STATE_DIR/${flat}."*.ok

  local bin="$BIN_DIR/$flat"
  local clog="$LOG_DIR/${flat}.compile.log"
  local rlog="$OUT_DIR/${flat}.out"

  if ! g++ -std="$std" -Wall -Wextra -O0 -g $flags "$src" -o "$bin" 2>"$clog"; then
    echo "FAIL-COMPILE  $rel  -> $clog"
    return 1
  fi

  # demo 以 nodemorun 标记时只验证编译
  if grep -q 'nodemorun' <<<"$hdr"; then
    echo "OK(compile-only)  $rel"
    : > "$ok_file"
    return 0
  fi

  if ! timeout 30 "$bin" >"$rlog" 2>&1; then
    local rc=$?
    # 有些 demo 故意以非 0 退出（演示异常/assert），标记 allowfail
    if grep -q 'allowfail' <<<"$hdr"; then
      echo "OK(exit=$rc, allowed)  $rel"
      : > "$ok_file"
      return 0
    fi
    echo "FAIL-RUN(exit=$rc)  $rel  -> $rlog"
    return 1
  fi

  echo "OK  $rel"
  : > "$ok_file"
  return 0
}
export -f run_one

mapfile -t SRCS < <(find "$DEMO_DIR" -name '*.cpp' | { [[ -n "$FILTER" ]] && grep -- "$FILTER" || cat; } | sort)

if [[ ${#SRCS[@]} -eq 0 ]]; then
  echo "没有找到待编译的 demo（filter='$FILTER'）"
  exit 0
fi

echo "共 ${#SRCS[@]} 个 demo，并行度 $JOBS"
printf '%s\0' "${SRCS[@]}" \
  | xargs -0 -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} \
  | tee "$BUILD_DIR/summary.txt"

fails=$(grep -c '^FAIL' "$BUILD_DIR/summary.txt" || true)
echo "----------------------------------------"
echo "成功: $(grep -c '^OK' "$BUILD_DIR/summary.txt" || true)  跳过: $(grep -c '^SKIP' "$BUILD_DIR/summary.txt" || true)  失败: $fails"
[[ "$fails" -eq 0 ]]

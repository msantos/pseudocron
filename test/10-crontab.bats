#!/usr/bin/env bats

@test "crontab format: minutes scheduled" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "*/5 * 26 * *"
cat << EOF
$output
EOF
  [ "$status" == "0" ]
  [ "$output" == "106902" ]
}

@test "crontab format: seconds scheduled" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "0 */5 * 26 * *"
cat << EOF
$output
EOF
  [ "$status" == "0" ]
  [ "$output" == "106902" ]
}

@test "crontab format: space delimited fields" {
  run pseudocron -n "  *    *  * *     *"
cat << EOF
$output
EOF
  [ "$status" == "0" ]
}

@test "crontab format: no tabs delimitng fields" {
  run pseudocron -n "*	* * * * *"
cat << EOF
$output
EOF
  [ "$status" == "1" ]
  [ "$output" == "pseudocron: invalid crontab timespec: Unsigned integer parse error 1" ]
}

@test "crontab format: invalid timespec" {
  run pseudocron -np "* 26 * *"
cat << EOF
$output
EOF
  [ "$status" == "1" ]
  [ "$output" == "pseudocron: invalid crontab timespec: Invalid number of fields, expression must consist of 6 fields" ]
}

@test "crontab alias: daily" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "@daily"
cat << EOF
$output
EOF
  [ "$status" == "0" ]
  [ "$output" == "20502" ]
}

@test "crontab alias: invalid alias" {
  run pseudocron -np "@foo"
cat << EOF
$output
EOF
  [ "$status" == "1" ]
  [ "$output" == "pseudocron: invalid crontab timespec" ]
}

@test "crontab alias: timespec too long" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 5 1 * * *"
cat << EOF
$output
EOF
  [ "$status" == "1" ]
  [ "$output" == "pseudocron: timespec exceeds maximum length: 252" ]
}

@test "crontab format: stdin: minutes scheduled" {
  run /bin/sh -c 'echo "*/5 * 26 * *" | pseudocron --stdin -np --timestamp="2018-01-24 18:18:18"'
cat << EOF
$output
EOF
  [ "$status" == "0" ]
  [ "$output" == "106902" ]
}

@test "crontab format: stdin: alias" {
  run /bin/sh -c 'echo "@hourly" | pseudocron --stdin -np --timestamp="2018-01-24 18:18:18"'
cat << EOF
$output
EOF
  [ "$status" == "0" ]
  [ "$output" == "2502" ]
}

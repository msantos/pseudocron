#!/usr/bin/env bats

@test "crontab format: minutes scheduled" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "*/5 * 26 * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 106902 ]
}

@test "crontab format: seconds scheduled" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "0 */5 * 26 * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 106902 ]
}

@test "crontab format: space delimited fields" {
  run pseudocron -n "  *    *  * *     *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
}

@test "crontab format: tab delimited fields" {
  run pseudocron -n "*	* * * * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
}

@test "crontab format: invalid timespec" {
  run pseudocron -np "* 26 * *"
cat << EOF
$output
EOF
  [ "$status" -eq 1 ]
  [ "$output" = "pseudocron: error: invalid crontab timespec: Invalid number of fields, expression must consist of 6 fields" ]
}

@test "crontab alias: daily" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "@daily"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 20502 ]
}

@test "crontab alias: invalid alias" {
  run pseudocron -np "@foo"
cat << EOF
$output
EOF
  [ "$status" -eq 1 ]
  [ "$output" = "pseudocron: error: invalid crontab timespec" ]
}

@test "crontab alias: timespec too long" {
  run pseudocron -np --timestamp="2018-01-24 18:18:18" "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 5 1 * * *"
cat << EOF
$output
EOF
  [ "$status" -eq 1 ]
  [ "$output" = "pseudocron: error: timespec exceeds maximum length: 252" ]
}

@test "crontab format: stdin: minutes scheduled" {
  run /bin/sh -c 'echo "*/5 * 26 * *" | pseudocron --stdin -np --timestamp="2018-01-24 18:18:18"'
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 106902 ]
}

@test "crontab format: stdin: alias" {
  run /bin/sh -c 'echo "@hourly" | pseudocron --stdin -np --timestamp="2018-01-24 18:18:18"'
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 2502 ]
}

@test "timestamp: daylight savings" {
  run pseudocron -np --timestamp "2018-03-12 1:55:00" "15 2 * * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 1200 ]
}

@test "timestamp: accept epoch seconds" {
  run pseudocron -np --timestamp "@1520834100" "15 2 * * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 1200 ]
}

@test "crontab format: spring daylight savings" {
  run pseudocron -np --timestamp "2019-03-09 11:43:00" "15 11 * * *"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
}

@test "crontab format: invalid day of month" {
  run pseudocron -np --timestamp "2019-03-09 11:43:00" "* * * 30 2 *"
cat << EOF
$output
EOF
  [ "$status" -eq 1 ]
}

@test "crontab format: @never" {
  run pseudocron -np "@never"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 4294967295 ]
}

@test "crontab format: @reboot: first run" {
  run pseudocron -np "@reboot"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 1 ]
}

@test "crontab format: @reboot: next run" {
  run env PSEUDOCRON_REBOOT=1 pseudocron -np "@reboot"
cat << EOF
$output
EOF
  [ "$status" -eq 0 ]
  [ "$output" -eq 4294967295 ]
}

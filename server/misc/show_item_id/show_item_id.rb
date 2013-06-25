#!/usr/bin/env ruby

$id = 0

def proc_one_line(line)
  if line =~ /^\t[A-Z0-9_]*,/
    puts sprintf("%04d: %s", $id, line)
    $id += 1
  else
    puts sprintf("----: %s", line)
  end
end

ARGF.each_line do |line|
  proc_one_line(line)
end


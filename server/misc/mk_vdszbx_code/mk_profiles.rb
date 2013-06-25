#!/usr/bin/env ruby


def print_one_group(line)
  expected_num_columns = 11
  columns = line.split("|")
  if columns.size != expected_num_columns then
    abort("Unexpected the number of columns: " + columns.to_s)
  end

  puts sprintf("\n\tgrp = table->addNewGroup();")
  puts sprintf("\tADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_PROFILEID, %s));", columns[1].strip)
  puts sprintf("\tADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_USERID,    %s));", columns[2].strip)
  puts sprintf("\tADD(new ItemString(ITEM_ID_ZBX_PROFILES_IDX, \"%s\"));", columns[3].strip)
  puts sprintf("\tADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_IDX2,      %s));", columns[4].strip)
  puts sprintf("\tADD(new ItemUint64(ITEM_ID_ZBX_PROFILES_VALUE_ID,  %s));", columns[5].strip)
  puts sprintf("\tADD(new ItemInt(ITEM_ID_ZBX_PROFILES_VALUE_INT,    %s));", columns[6].strip)
  puts sprintf("\tADD(new ItemString(ITEM_ID_ZBX_PROFILES_VALUE_STR, \"%s\"));", columns[7].strip)
  puts sprintf("\tADD(new ItemString(ITEM_ID_ZBX_PROFILES_SOURCE,    \"%s\"));", columns[8].strip)
  puts sprintf("\tADD(new ItemInt(ITEM_ID_ZBX_PROFILES_TYPE,         %s));", columns[9].strip)

end

while line = STDIN.gets
  print_one_group(line)
end


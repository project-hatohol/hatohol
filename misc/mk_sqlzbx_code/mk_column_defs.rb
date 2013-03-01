#!/usr/bin/env ruby

SQL_TYPE_MAP = {
  "bigint unsigned" => "SQL_COLUMN_TYPE_BIGUINT",
  "int"             => "SQL_COLUMN_TYPE_INT",
  "char"            => "SQL_COLUMN_TYPE_CHAR",
  "varchar"         => "SQL_COLUMN_TYPE_VARCHAR",
  "text"            => "SQL_COLUMN_TYPE_TEXT"
}

def print_one_group(line)
  expected_num_columns = 8
  columns = line.split("|")
  if columns.size != expected_num_columns
    abort("Unexpected the number of columns: #{columns.join("|")}")
  end

  # Item Name
  item_name = columns[1].strip;

  # SQLColumnType
  sql_type_key = columns[2].sub(/\([0-9]+\)/, "").strip
  sql_type = SQL_TYPE_MAP[sql_type_key]
  digit = columns[2].sub(/[^0-9]+/, "")
  digit = digit.sub(/[^0-9]+/, "")
  if digit == ""
    digit = 0
  end

  # Can be null
  can_null_str = columns[3].strip
  case can_null_str
  when "YES"
    can_be_null = "true"
  when "NO"
    can_be_null = "false"
  else
    abort("Unexpected value for canBeNull: #{can_null_str}")
  end

  # key type
  key_type_str = columns[4].strip
  case key_type_str
  when "PRI"
    key_type = "SQL_KEY_PRI"
  when "MUL"
    key_type = "SQL_KEY_MUL"
  when "UNI"
    key_type = "SQL_KEY_UNI"
  when ""
    key_type = "SQL_KEY_NONE"
  else
    abort("Unexpected value for the key type: #{key_type_str}")
  end

  # default value
  default_value_str = columns[5].strip
  if default_value_str == "NULL"
    default_value = "NULL"
  else
    default_value = %Q!"#{default_value_str}"!
  end

  puts sprintf("\tdefineColumn(staticInfo, ITEM_ID_%s,", item_name.upcase)
  puts sprintf("\t             TABLE_ID, \"%s\",", item_name)
  puts sprintf("\t             %s, %d,", sql_type, digit)
  puts sprintf("\t             %s, %s, %s);", can_be_null, key_type, default_value)

#  defineColumn(staticInfo, ITEM_ID_ZBX_ITEMS_ITEMID,
#               TABLE_ID_ITEMS, "itemid",
#               SQL_COLUMN_TYPE_BIGUINT, 20,
#               false, SQL_KEY_PRI, NULL);
end

ARGF.each_line do |line|
  print_one_group(line)
end


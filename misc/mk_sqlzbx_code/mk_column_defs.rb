#!/usr/bin/env ruby

$sql_type_map = {
  "bigint unsigned" => "SQL_COLUMN_TYPE_BIGUINT",
  "int"             => "SQL_COLUMN_TYPE_INT",
  "char"            => "SQL_COLUMN_TYPE_CHAR",
  "varchar"         => "SQL_COLUMN_TYPE_VARCHAR",
  "text"            => "SQL_COLUMN_TYPE_TEXT"
}

def print_one_group(line)
  expected_num_columns = 8
  columns = line.split("|")
  if columns.size != expected_num_columns then
    abort("Unexpected the number of columns: " + columns.to_s)
  end

  # Item Name
  item_name = columns[1].strip;

  # SQLColumnType
  sql_type_key = columns[2].sub(/\([0-9]+\)/, "").strip
  sql_type = $sql_type_map[sql_type_key]
  digit = columns[2].sub(/[^0-9]+/, "")
  digit = digit.sub(/[^0-9]+/, "")

  # Can be null
  can_null_str = columns[3].strip
  if can_null_str == "YES"
    can_be_null = "true"
  elsif can_null_str == "NO"
    can_be_null = "false"
  else
    abort("Unexpected value for canBeNull: " + can_null_str)
  end

  # key type
  key_type_str = columns[4].strip
  if key_type_str == "PRI"
    key_type = "SQL_KEY_PRI"
  elsif key_type_str == "MUL"
    key_type = "SQL_KEY_MUL"
  elsif key_type_str == "UNI"
    key_type = "SQL_KEY_UNI"
  elsif key_type_str == ""
    key_type = "SQL_KEY_NONE"
  else
    abort("Unexpected value for the key type: " + key_type_str)
  end

  # default value
  defalut_value_str = columns[5].strip
  if defalut_value_str == "NULL"
    default_value = "NULL"
  else
    default_value = '"' + defalut_value_str + '"'
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

while line = STDIN.gets
  print_one_group(line)
end


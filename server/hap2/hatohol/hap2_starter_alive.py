#!/usr/bin/env python
#
# Copyright (C) 2016 MIRACLE LINUX CORPORATION All Rights Reserved.

DEFAULT_LOG_LEVEL = "WARNING"
HAP2_COUNT_QUERY = "select count(*) from arm_plugins where path <> '';"
CONFIG_FILE="/etc/hatohol/hatohol.conf"

import os
from sys import exit
import logging
import MySQLdb
import ConfigParser
import argparse
import subprocess
import traceback

config_map = {
    'database': 'hatohol',
    'user': 'hatohol',
    'password': 'hatohol',
    'host': 'localhost',
    'port': 0
}

def init_config_map():
    if not os.access(CONFIG_FILE, os.R_OK):
        logging.debug("Can not access %s, use default config_map" % CONFIG_FILE)
        return
    logging.debug("Use %s values as default." % CONFIG_FILE)

    config = ConfigParser.ConfigParser()
    config.read(CONFIG_FILE)
    data = (
        ('database', 'mysql', 'database'),
        ('user', 'mysql', 'user'),
        ('password','mysql', 'password'),
        ('host', 'mysql', 'host'),
        ('port', 'mysql', 'port'),
    )
    global config_map
    for key, section, option in data:
        if not config.has_option(section, option):
            continue
        config_map[key] = config.get(section, option)

def get_default_conf_item(item_name):
    return config_map.get(item_name)

def setup_commandline_args():
    # Default values are hatohol.conf values.
    parser = argparse.ArgumentParser(description='Hatohol arm plugin starter counter')
    parser.add_argument('--db-name', type=str,
                        default=get_default_conf_item('database'),
                        help='A database name to be initialized.')
    parser.add_argument('--host', type=str,
                        default=get_default_conf_item('host'),
                        help='A database server.')
    parser.add_argument('--port', type=int,
                        default=get_default_conf_item('port'),
                        help='A database port.')
    parser.add_argument('--user', type=str,
                        default=get_default_conf_item('user'),
                        help='A user who is allowed to access the database.')
    parser.add_argument('--password', type=str,
                        default=get_default_conf_item('password'),
                        help='A password that is used to access the database.')
    return parser.parse_args()

def exec_sql(args, sql_str):
    try:
        for item in vars(args).items():
            logging.debug(item)
        conn = MySQLdb.connect(
            host   = args.host,
            port   = int(args.port),
            user   = args.user,
            passwd = args.password,
            db     = args.db_name
        )
        logging.debug("conn: %s" % str(conn))
        cursor = conn.cursor()
        logging.debug("cursor: %s" % str(cursor))

        logging.debug("sql_str: %s" % sql_str)
        cursor.execute(sql_str)
        result = cursor.fetchone()
        logging.debug("result exec_sql: " + str(result))
        cursor.close()
        return result
    except:
        logging.error("Exception is occured when execute exex_sql:\n%s\n"
                        % traceback.format_exc())
        exit(0)

def get_required_number(args):
    result = exec_sql(args, HAP2_COUNT_QUERY)
    if 1 != len(result):
        logging.error("query result has too many columns")
        exit(1)

    required_number = int(result[0])
    logging.debug("required_count: " + str(required_number))

    return required_number

def is_correct_hap2_number(required_number):
    try:
        hatohol_pgid = \
                subprocess.check_output(["pgrep", "hatohol"])[:-1]
        logging.debug("Hatohol pgid: %s" % hatohol_pgid)
        p1 = subprocess.Popen(["ps", "-eo" "pgid,command"],
                              stdout=subprocess.PIPE)
        p2 = subprocess.Popen(["grep", "-w", hatohol_pgid],
                              stdin=p1.stdout,
                              stdout=subprocess.PIPE)
        exact_number = subprocess.check_output(["grep", "-c", "hap2_starter"],
                                               stdin=p2.stdout)
    except subprocess.CalledProcessError:
        logging.error("Any subprocess got non zero exit code.")
        logging.error(traceback.format_exc())
        exit(1)
    except:
        logging.error("Unexpected error: %s\n" % traceback.format_exc())
        exit(1)

    if required_number == int(exact_number):
        return True
    else:
        return False


# execute myself
if __name__ == "__main__":
    logging.basicConfig(
        level    = getattr(logging, DEFAULT_LOG_LEVEL, None),
        format   = "%(asctime)s %(levelname)s %(lineno)d %(message)s"
    )
    logging.debug("Set up logging")

    init_config_map()
    args = setup_commandline_args()

    required_number = get_required_number(args)
    if required_number == 0:
        logging.debug("hap2 is not registered in Hatohol server.")
        exit(0)

    if is_correct_hap2_number(required_number):
        logging.debug("All hap2 runnning correctly.")
        exit(0)
    else:
        logging.error("Some hap2 causing the problem.")
        exit(1)

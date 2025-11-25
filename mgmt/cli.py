import redis
import json
import argparse
import time
import sys
from prettytable import PrettyTable

r = redis.Redis(host='localhost', port=6379, db=0, decode_responses=True)

def get_json(key):
    value = r.get(key)
    if value:
        return json.loads(value)
    return None

def cmd_show_interfaces():
    table = PrettyTable()
    table.field_names = ["Interface", "Admin", "Oper", "Link", "Last Change"]
    
    # scan for all interfaces based on OPER status keys
    keys = r.keys("sysdb/interface/*/status")
    for key in keys:
        # sysdb/interface/eth1/status
        parts = key.split("/")
        if len(parts) < 4: continue
        intf = parts[2]
        
        # Get operational status
        oper_data = get_json(key)
        oper_status = oper_data.get("operStatus", "unknown") if oper_data else "unknown"
        link_status = oper_data.get("linkStatus", "unknown") if oper_data else "unknown"
        last_changed = oper_data.get("lastChanged", "") if oper_data else ""
        
        # Get admin config
        config_key = f"sysdb/interface/{intf}/config"
        config_data = get_json(config_key)
        admin_enabled = config_data.get("adminEnabled", False) if config_data else False
        admin_str = "enabled" if admin_enabled else "disabled"
        
        table.add_row([intf, admin_str, oper_status, link_status, last_changed])
        
    print(table)

def cmd_config_intf(interface, state):
    valid_states = ["up", "down"]
    if state not in valid_states:
        print(f"Invalid state '{state}'. Valid states are: {valid_states}")
        return
    
    key = f"sysdb/interface/{interface}/config"
    
    # Use boolean for adminEnabled 
    payload = {
        "adminEnabled": state == "up",
        "user": "admin",
        "lastChanged": int(time.time())
    }
    r.set(key, json.dumps(payload))
    admin_str = "enabled" if state == "up" else "disabled"
    print(f"Interface {interface} admin state set to {admin_str}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MiniSwitchOS CLI")
    subparsers = parser.add_subparsers(dest="command")

    # show command
    p_show = subparsers.add_parser("show", help="Show system state")
    p_show.add_argument("entity", choices=["interfaces"], help="What to show")

    # config command
    p_conf = subparsers.add_parser("config", help="Configure system")
    p_conf.add_argument("interface", help="Interface name (ex: eth1)")
    p_conf.add_argument("state", choices=["up", "down"], help="Admin state")

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    args = parser.parse_args()

    if args.command == "show":
        if args.entity == "interfaces":
            cmd_show_interfaces()
    
    elif args.command == "config":
        cmd_config_intf(args.interface, args.state)
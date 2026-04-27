#!/usr/bin/env python3
"""Flash a coreboot binary to a free SnipeIT-managed DUT via osfv_cli.

Looks up a Ready-to-Deploy, unassigned asset for the given model in SnipeIT,
reads its RTE IP from a custom field, checks the asset out to the calling
user, runs `osfv_cli rte --rte_ip <ip> flash write --rom <rom>`, then checks
the asset back in (always, even if the flash fails).

Env vars:
    SNIPEIT_IP       SnipeIT host IP/hostname (default: 192.168.4.202)
    SNIPEIT_API_KEY  Personal API token (required)
    SNIPEIT_UID      Numeric user ID to check out to. Optional; if unset,
                     falls back to GET /api/v1/users/me (newer SnipeIT only).
"""

import argparse
import os
import subprocess
import sys

import requests


SNIPEIT_IP = os.environ.get("SNIPEIT_IP", "192.168.4.202").strip().rstrip("/")
SNIPEIT_URL = f"http://{SNIPEIT_IP}"
SNIPEIT_API_KEY = os.environ.get("SNIPEIT_API_KEY")
READY_TO_DEPLOY = "Ready to Deploy"


def api(method, path, **kwargs):
    if not SNIPEIT_API_KEY:
        sys.exit("SNIPEIT_API_KEY env var is required")
    headers = kwargs.pop("headers", {})
    headers.update({
        "Authorization": f"Bearer {SNIPEIT_API_KEY}",
        "Accept": "application/json",
        "Content-Type": "application/json",
    })
    r = requests.request(
        method, f"{SNIPEIT_URL}/api/v1{path}",
        headers=headers, timeout=30, **kwargs,
    )
    r.raise_for_status()
    data = r.json()
    # SnipeIT returns HTTP 200 with {"status":"error",...} on logical errors.
    if isinstance(data, dict) and data.get("status") == "error":
        sys.exit(f"SnipeIT error on {method} {path}: {data.get('messages')}")
    return data


def find_model_id(name):
    data = api("GET", "/models", params={"search": name, "limit": 50})
    rows = data.get("rows", [])
    exact = [m for m in rows if m["name"].lower() == name.lower()]
    matches = exact or rows
    if not matches:
        sys.exit(f"No SnipeIT model matches '{name}'")
    if len(matches) > 1:
        names = ", ".join(sorted(m["name"] for m in matches))
        sys.exit(f"Ambiguous model name '{name}': {names}")
    return matches[0]["id"]


def find_free_asset(model_id):
    data = api("GET", "/hardware", params={
        "model_id": model_id,
        "status": "RTD",
        "limit": 100,
    })
    for asset in data.get("rows", []):
        status = (asset.get("status_label") or {}).get("name")
        if status != READY_TO_DEPLOY:
            continue
        if asset.get("assigned_to"):
            continue
        return asset
    sys.exit(
        f"No '{READY_TO_DEPLOY}' unassigned asset found for model_id={model_id}"
    )


def get_rte_ip(asset):
    fields = asset.get("custom_fields") or {}
    for key, payload in fields.items():
        k = key.lower()
        if "rte" in k and "ip" in k:
            value = (payload or {}).get("value")
            if value:
                return value
    sys.exit(f"Asset {asset.get('asset_tag')} has no RTE IP custom field")


def get_my_user_id():
    env = os.environ.get("SNIPEIT_UID")
    if env:
        return int(env)
    return api("GET", "/users/me")["id"]


def checkout(asset_id, user_id):
    api("POST", f"/hardware/{asset_id}/checkout", json={
        "checkout_to_type": "user",
        "assigned_user": user_id,
        "note": "flash_via_snipeit.py auto-checkout",
    })


def checkin(asset_id):
    api("POST", f"/hardware/{asset_id}/checkin", json={
        "note": "flash_via_snipeit.py auto-checkin",
    })


def flash(rte_ip, rom_path):
    cmd = ["osfv_cli", "rte", "--rte_ip", rte_ip,
           "flash", "write", "--rom", rom_path]
    print("+ " + " ".join(cmd), flush=True)
    return subprocess.run(cmd, check=False).returncode


def main():
    ap = argparse.ArgumentParser(
        description="Flash a coreboot binary to a SnipeIT-managed DUT."
    )
    ap.add_argument("rom", help="Path to coreboot binary to flash")
    ap.add_argument("model", help="Platform/model name as it appears in SnipeIT")
    args = ap.parse_args()

    if not os.path.isfile(args.rom):
        sys.exit(f"ROM file not found: {args.rom}")

    user_id = get_my_user_id()
    model_id = find_model_id(args.model)
    asset = find_free_asset(model_id)
    rte_ip = get_rte_ip(asset)
    asset_id = asset["id"]
    asset_tag = asset["asset_tag"]
    print(f"Selected asset {asset_tag} (id={asset_id}), RTE IP {rte_ip}")

    print(f"Checking out {asset_tag} to user_id={user_id}...")
    checkout(asset_id, user_id)
    try:
        rc = flash(rte_ip, args.rom)
    finally:
        print(f"Checking in {asset_tag}...")
        try:
            checkin(asset_id)
        except Exception as e:
            print(f"WARNING: check-in failed: {e}", file=sys.stderr)
    sys.exit(rc)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Speak or announce arbitrary text on YOUR OWN Echo via alexapy.

A minimal, adaptable starting point for Python-based Echo automation. It logs in
with the account owner's credentials (the same login the Alexa app uses) and
sends text-to-speech or an announcement to a named device.

Only use this against an Amazon account and devices you own or are authorized to
manage. Credentials are read from the environment, never hard-coded.

Requirements:
    pip install alexapy
Environment:
    ALEXA_EMAIL     Amazon account email
    ALEXA_PASSWORD  Amazon account password
    ALEXA_DOMAIN    Amazon marketplace domain (default amazon.es)
    ALEXA_OUTPUT    directory to cache the login cookie (default ~/.alexa)

Usage:
    python alexa_tts.py --device "Echo Salón" --message "Hola"
    python alexa_tts.py --device "Echo Salón" --message "La cena" --mode announce
    python alexa_tts.py --list        # list device names, then exit

First run opens a proxy login (follow the printed URL) to complete 2FA once; the
cookie is cached in ALEXA_OUTPUT for subsequent runs.
"""
import argparse
import asyncio
import os
import sys

try:
    from alexapy import AlexaLogin, AlexaAPI
except ImportError:
    sys.exit("alexapy is not installed. Run: pip install alexapy")


async def run(args: argparse.Namespace) -> int:
    email = os.environ.get("ALEXA_EMAIL")
    password = os.environ.get("ALEXA_PASSWORD")
    domain = os.environ.get("ALEXA_DOMAIN", "amazon.es")
    outputpath = os.environ.get("ALEXA_OUTPUT", os.path.expanduser("~/.alexa"))

    if not email or not password:
        sys.exit("Set ALEXA_EMAIL and ALEXA_PASSWORD in the environment.")

    os.makedirs(outputpath, exist_ok=True)
    login = AlexaLogin(domain, email, password, outputpath=outputpath)
    await login.login()
    if login.status.get("login_successful") is not True:
        # A proxy/2FA step may be required; alexapy prints the URL to visit.
        print("Login not complete — follow the proxy URL above to finish 2FA, "
              "then re-run.", file=sys.stderr)
        return 2

    devices = await AlexaAPI.get_devices(login)
    names = [d.get("accountName", "?") for d in devices]

    if args.list:
        print("\n".join(sorted(names)))
        return 0

    match = next((d for d in devices if d.get("accountName") == args.device), None)
    if match is None:
        print(f"Device {args.device!r} not found. Available:", file=sys.stderr)
        print("\n".join(f"  - {n}" for n in sorted(names)), file=sys.stderr)
        return 1

    api = AlexaAPI(match, login)
    if args.mode == "announce":
        await api.send_announcement(args.message)
    else:
        await api.send_tts(args.message)
    print(f"Sent ({args.mode}) to {args.device!r}: {args.message!r}")
    return 0


def main() -> None:
    p = argparse.ArgumentParser(description="Speak/announce on your own Echo.")
    p.add_argument("--device", help="Exact device name (see --list)")
    p.add_argument("--message", help="Text to speak or announce")
    p.add_argument("--mode", choices=["tts", "announce"], default="tts")
    p.add_argument("--list", action="store_true", help="List devices and exit")
    args = p.parse_args()

    if not args.list and (not args.device or not args.message):
        p.error("--device and --message are required unless --list is given")

    raise SystemExit(asyncio.run(run(args)))


if __name__ == "__main__":
    main()

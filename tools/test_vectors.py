#!/usr/bin/env python3
import argparse, json, pathlib, re, struct, subprocess, sys, urllib.request

ROOT=pathlib.Path(__file__).resolve().parents[1]
URL="https://raw.githubusercontent.com/SingleStepTests/65x02/main/6502/v1/{:02x}.json"

def emit(reg, ram):
    out=struct.pack("<HBBBBB",reg["pc"],reg["s"],reg["a"],reg["x"],reg["y"],reg["p"])+struct.pack("<H",len(ram))
    for address,value in ram: out+=struct.pack("<HB",address,value)
    return out

def main():
    ap=argparse.ArgumentParser(description="Run SingleStepTests 6502 JSON vectors")
    ap.add_argument("files",nargs="*",type=pathlib.Path)
    ap.add_argument("--download",action="store_true",help="download vectors for every opcode implemented by this core")
    args=ap.parse_args();files=args.files
    if args.download:
        cache=ROOT/"tests"/"vectors";cache.mkdir(parents=True,exist_ok=True)
        source=(ROOT/"src"/"opcode.c").read_text();opcodes=sorted({int(x,16) for x in re.findall(r"D\(0x([0-9A-Fa-f]{2}),",source)})
        for opcode in opcodes:
            path=cache/f"{opcode:02x}.json"
            if not path.exists():print(f"fetching {path.name}",file=sys.stderr);urllib.request.urlretrieve(URL.format(opcode),path)
            files.append(path)
    if not files:ap.error("provide JSON files or --download")
    runner=subprocess.Popen([str(ROOT/"bin"/"run_vectors")],stdin=subprocess.PIPE);assert runner.stdin
    for path in files:
        for case in json.loads(path.read_text()):
            initial,final=case["initial"],case["final"]
            if len(final["ram"])>64:raise ValueError("vector has more than 64 final RAM entries")
            runner.stdin.write(emit(initial,initial["ram"]));runner.stdin.write(emit(final,final["ram"]));runner.stdin.write(bytes([len(case["cycles"])]))
    runner.stdin.close();return runner.wait()
if __name__=="__main__":raise SystemExit(main())

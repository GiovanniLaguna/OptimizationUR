import subprocess
import sys

cmd = [
    r"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    r"c:\Users\Gio\Documents\Unreal Projects\Project_URO\Project_URO.uproject",
    r"-ExecutePythonScript=c:\Users\Gio\Documents\Unreal Projects\Project_URO\scratch\verify_locomotion.py",
    "-stdout",
    "-FullStdOutLogOutput",
    "-unattended"
]

print("Launching UnrealEditor-Cmd...")
proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding='latin1')

found = False
for line in proc.stdout:
    if "LogPython" in line or "VERIFYING" in line or "Mesh:" in line or "AnimClass:" in line:
        print(line.strip())
        found = True

proc.wait()
print(f"Process finished with returncode: {proc.returncode}")

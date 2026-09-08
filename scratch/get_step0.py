import json

with open(r'C:/Users/Gio/.gemini/antigravity-ide/brain/cac2bec6-61d2-45f1-9338-2f81c9934b69/.system_generated/logs/transcript.jsonl', 'r', encoding='utf-8') as f:
    first_line = f.readline()
    d = json.loads(first_line)
    with open(r'c:/Users/Gio/Documents/Unreal Projects/Project_URO/scratch/step0.txt', 'w', encoding='utf-8') as out:
        out.write(d.get('content', ''))

print("Wrote step0.txt successfully")

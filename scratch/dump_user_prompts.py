import json

with open(r'C:/Users/Gio/.gemini/antigravity-ide/brain/cac2bec6-61d2-45f1-9338-2f81c9934b69/.system_generated/logs/transcript.jsonl', 'r', encoding='utf-8') as f:
    for line in f:
        try:
            d = json.loads(line)
            if d.get('type') == 'USER_INPUT':
                print(f"=== Step {d.get('step_index')} ===")
                print(d.get('content'))
                print("-" * 50)
        except Exception as e:
            pass

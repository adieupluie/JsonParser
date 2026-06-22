간단 안내: VS Code를 한국어로 설정하기

1) 자동 스크립트 사용 (권장)
PowerShell에서 다음을 실행:

```powershell
cd "${PWD}"
.\setup_vscode_korean.ps1
```

2) 수동 방법
- 확장 보기(`Ctrl+Shift+X`)에서 "Korean Language Pack" 또는 "Korean Language Pack for Visual Studio Code" 설치 (확장 ID: `ms-ceintl.vscode-language-pack-ko`).
- `Ctrl+Shift+P` → `Configure Display Language` → `ko` 선택 → VS Code 재시작.
- 또는 사용자 설정 파일에 `%APPDATA%\Code\User\locale.json`을 만들고 아래 내용 저장:

```json
{
  "locale": "ko"
}
```

3) 참고
- 일부 확장과 메시지는 번역되지 않을 수 있습니다.
- `code` 명령어가 없으면 VS Code에서 Command Palette로 설치하거나, 설치 경로를 PATH에 추가하세요.

# JSONTreeViewer (Win32 C++)

이 프로젝트는 Visual Studio 2017에서 빌드 가능한 Win32 C++ 기반 JSON 트리 뷰어입니다.

## 요구 사항

- Visual Studio 2017
- Windows SDK

## 빌드 방법

1. Visual Studio 2017에서 `JSONTreeViewer.sln`을 엽니다.
2. 솔루션 플랫폼을 `x64`로 선택하고 `Release` 또는 `Debug`로 빌드합니다.

또는 VS Code에서 빌드하려면:

- VS 설치가 되어 있으면 "Developer Command Prompt for VS2017"를 엽니다(또는 Visual Studio 명령 프롬프트).
- 해당 프롬프트에서 다음을 실행하세요:

```powershell
cd c:\workspace\VisualCode\JSONTreeViewer
msbuild JSONTreeViewer.sln /p:Configuration=Release /p:Platform=x64
```

문제가 발생할 때 확인할 것:

- `msbuild` 또는 `cl.exe`가 인식되지 않으면 Visual Studio 빌드 도구가 설치되어 있지 않거나 환경 변수가 설정되지 않은 상태입니다.
- VS Code의 `tasks.json`는 기본적으로 VS2017의 `VsDevCmd.bat` 경로를 사용하도록 되어 있습니다. 설치 경로가 다르면 `.vscode/tasks.json`의 `VsDevCmd.bat` 호출 경로를 실제 경로로 수정하세요.

예: `C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat`

문제가 계속되면 빌드 출력(오류 메시지 전체)을 공유해주시면 원인 진단과 수정사항을 적용하겠습니다.

## 기능

- JSON 텍스트 입력
- `Ctrl+P`로 파싱
- 파싱 결과를 트리 뷰로 표시
- `Open`, `Save` 메뉴 지원
- 오류 메시지 표시

# Object Hub QT  
  
Claude:  
Native Qt Widgets client for objecthub.xyz (openGo protocol V133R3).  
Checked on Qt 5.15 and Qt 6.11. Qt 5.6 (Windows XP) is a target but not yet verified; see docs/ARCHITECTURE.md.  
  
MIOBOMB: I absolutely don't know how it works!! :(  
  

## Platforms  
  
Tier 1 - testing before every release:  
- FreeBSD  
  
Tier 2 - working, bugfixes only:  
- Modern Windows  
- any GNU/Linux  
- BSD systems (OpenBSD, NetBSD, etc.)
  
Tier 3 - planned, may not work:  
- Windows XP  
- Windows 7  
- Haiku OS  
- Android  
- macOS  
- iOS  
  

## Architecture  
  
Claude:  
The UI talks to `ApiClient` (`src/api/`), a facade over domain objects in `src/domain/`  
(account, catalog, detail, profile, social, projects, vacancies, news, portfolio). Shared  
transport and protocol code is in `src/core/`. See docs/ARCHITECTURE.md.  
  
MIOBOMB: i hate it  
  

## Build (desktop)  
`qmake ObjectHubClient.pro`  
`make`  
  
Strings: resources/lang/RU.json (site l10n pack). Inline UI strings marked // LANGXXX are client-only  
  
## ALTCHA  
Login/register need the plain-C solver under src/altcha/  

## UI Design Philosophy  
ojhub-qt is a fully native client without QML. It uses the system UI (Qt Widgets) for speed and a fully native look  

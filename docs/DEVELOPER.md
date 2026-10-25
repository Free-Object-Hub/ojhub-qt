# Object Hub Qt client: developer notes

Written for a reader who has never read the code. It explains what is where, how a
request travels from a button to the server and back, and what the server expects.
Protocol facts come from openGo (`/server/133/`), which is the reference.

## 1. Build and run

- Checked on Qt 5.15. Targets are Qt 5.6 (Windows XP) and Qt 6 (FreeBSD); neither is verified yet.
  Keep APIs present in both. Known Qt 6 removals are already avoided: no `QDateTime::fromTime_t`, no `QRegExp`.
- `qmake6 .. && make -j4`, or open `ObjectHubClient.pro` in Qt Creator.
- The app keeps its files next to the binary: `ObjectHub.ini` (device id) and `cache/`
  (avatar cache, 50 MB limit). Nothing is written to the registry or to the user profile.

## 2. Folder map

| Path | What it is |
|---|---|
| `src/main.cpp` | Starts the application, loads `resources/lang/RU.json`. |
| `src/mainwindow.*` | The main window: tabs, search, the detail page, menus, all user actions. Biggest file. |
| `src/resultsmodel.*` | List model for search and news results, loads avatars with the disk cache. |
| `src/profileview.*` | Dialog that shows a profile (older view, still used). |
| `src/logindialog.*`, `src/registerdialog.*` | Login and registration forms. |
| `src/projectdialog.*` | Create or edit a camp, show, pere or tele. |
| `src/vacancydialog.*` | Create or edit a vacancy. |
| `src/newsdialog.*` | Create or edit a news post. |
| `src/socialdialogs.*` | Profile editor, subscriptions list, notifications list. |
| `src/tagpicker.*` | Checkboxes for tags and OS, names come from `RU.json`. |
| `src/settings.*` | Paths next to the binary and the persistent device id. |
| `src/lang.*` | Translations: `L("key")` returns the text, or the key if missing. |
| `src/version.h` | `OH_VERSION`, shown in the window title. |
| `src/api/apiclient.*` | The façade the UI calls. Forwards to the domains and re-emits their signals. |
| `src/domain/*/` | One folder per domain: requests, answer parsing, signals. See `docs/ARCHITECTURE.md`. |
| `src/core/*` | Session (tokens), Network (HTTP, timeouts), protocol helpers. |
| `src/api/forms.h` | Plain structs for the edit forms and the list of write actions (`WriteAction`). |
| `src/api/searchresult.h` | `SearchResult`, `DetailData`, `PublicProfile`, the channel codes. |
| `resources/lang/RU.json` | The site's Russian dictionary (copied from 0.98.2). Do not change its keys. |

## 3. How a request travels

Example: the user presses "Редактировать" on a project.

1. `MainWindow::onEditDetail()` runs. It reads the current item from `m_history.last()`.
2. It calls `m_api->fetchProjectForm(channel, id)`. `ApiClient` forwards to
   `ProjectsDomain::fetchProjectForm`, which builds the URL, tags the reply with `tagReply()`
   (action, id, channel), and connects `finished` to its own `onReplyFinished()`.
3. The reply arrives. `ProjectsDomain::onReplyFinished()` reads the tags, parses the body and emits
   `projectFormLoaded`, or `actionFinished(action, ok, message, id)` on error.
4. `ApiClient` re-emits the signal, and `MainWindow::onProjectFormLoaded()` opens `ProjectDialog`.
   When the user saves, `m_api->saveProject(...)` starts the same kind of request again.

Where to look: the request function and its reply handling live in the same domain file. The UI
decides what to do with the result in `mainwindow.cpp`. If a request changes state that another
domain owns (for example, the owned-projects list after a project save), the domain emits a signal
and `ApiClient` asks the right domain to act. It does not reach into the other domain.

## 4. The detail page

`m_history` is a stack of `DetailData`. Each open item is one entry, and "Назад" pops it.
`showTopDetail()` renders the top entry into the text browser, then calls `updateActions()`
(buttons above the text) and `ensureProbes()` (asks the server for what the page cannot know).

Links inside the text are `oh://` URLs. `MainWindow::onDetailAnchor()` reads them:

| Link | Action |
|---|---|
| `oh://proj/ID`, `oh://vac/ID`, `oh://news/ID`, `oh://user/ID` | Open an item or a profile. |
| `oh://vote/TYPE/ID/like` or `dislike` | Like or dislike. TYPE is in `itemCodeFor` / `commentCodeFor`. |
| `oh://commwrite/CONTENT/ID` | Write a comment under an item. |
| `oh://cedit/ID`, `oh://cdel/ID` | Edit or delete a comment (own comments only shown). |
| `oh://newsnew/PROJECT` | New news post in a project. |
| `oh://profedit/0`, `oh://subs/0`, `oh://alarms/0` | Own profile actions. |

Buttons above the text are created in `setupUi()`. Their visibility comes only from
`updateActions()`.

## 5. Owner checks and probes

The client does not know who owns a vacancy or a news post from the page data alone, so it
asks the server with a request that changes nothing:

- **Vacancy:** `GET vacans/edit` answers with the vacancy for its owner and `-2` for everyone else
  (`probeVacancy`).
- **News:** `POST newsModify` with an empty title. The server checks access first. Owners get an
  empty answer and nothing is written. Others get `-2` (`probeNews`). This is a write endpoint,
  so keep the empty title; do not "fix" it by sending real text.
- **Subscription on a project:** `gdps/subs` is paged. The client walks the pages until it finds
  the project (`fetchSubscriptions(page, probeId)`). This can take several requests for a user
  with many subscriptions.

Answers are kept in the window (`m_vacOwner`, `m_newsOwner`, `m_subState`), keyed by object id. They are
cleared at login and logout. Value -1 means "asked, no answer yet", so each object is asked once.

## 6. Server answers you will meet

openGo answers in several shapes. The parsers accept all of them:

- A **JSON array** for lists and for "changed" answers (InitOjhub after project save).
- A **plain number** as text for new ids (`"12"`), or `"1"` for success.
- **Negative codes** as text: `-1` server failed, `-2` no access, `-3` bad image or missing object,
  `-4` too short, `-27`, `-28`, `-29` bad title or description. See `serverErrorText()`.
- **`Access denied`** when the device or token is wrong.
- **`["NONE"]`** when an object does not exist.
- **`{"error": "...", "code": "-28"}`** from a few ApiError handlers.

Some endpoints read `ParseForm()` before reading fields. For those, the body must be urlencoded,
not multipart. The list is in each domain's request functions (search for "urlencoded" in `src/domain/`).
Endpoints that read `ParseMultipartForm` need multipart: `vacsEdit`, project save, news add.

Stored text has two quirks:
- Resume and socials keep newlines as the two characters `\n`. Editors use real newlines,
  `storedToView()` and `ProfileEditDialog` convert both ways.
- Comments and news come back decoded. Project descriptions can be JSON maps keyed by language
  (`{"RU": "..."}`); `parseMultiLang()` handles both forms.

### Paging contract

Lists are paged by offset: the server runs `LIMIT 9 OFFSET 8*page` for search and vacancies
(8 per page) and `LIMIT 11 OFFSET 10*page` for news (10 per page). The client asks for `page`, and
the server returns one row more than a page. That extra row is only the "there is more" marker: the
client shows the first `pageSize` rows and sets `hasMore`. The next page starts at the marker,
so the marker is never lost.

Object answers (projects search) are keyed `c12`, `s3`, `p7`, `t1`. The order of the keys is the
server's ranking, so the client reads the keys in text order (`topLevelKeysInOrder`), not with
`QJsonObject::keys()`, which sorts them.

Only one list request is alive at a time (search, news or vacancies). A new one aborts the old one.

### Tags

Tags and OS are one numeric space per section, stored in two fields and normalised the same way.
The dictionary (`src/tagnames.cpp`) mirrors the site's JS tables:

| Section | Tags | OS | Names |
|---|---|---|---|
| camp (0) | 1–10 | 11–15 | `Camptag*` |
| show (1) | 1–9 | 12–15 | `Showtag*` |
| pere (2) | 1–9 | 12–15 | `Peretag*` (same texts as show) |
| tele (3) | 1–9 | 12–15 | `Teletag*` |
| vacancies | 1–10 | none | `Vacstag*` |

Search takes tags only for one concrete section. Showing an id that is not in the dictionary falls
back to the bare number. The OS field name (`os`) in the search answer is an assumption: it is
covered by a test, but not yet checked against a live answer.

## 7. Channel and like codes

Projects: camp 0, show 1, pere 2, tele 3. Search can use -10 for "all". Vacancies use -5 and news
uses -20, but those two exist only in the client.

Likes on the server (`legacy.go`, `LikeTypes`):
item project 0, news 2, vacancy 11; comment on a project 3, on news 5, on a vacancy 12.

Comment content types, used when posting or loading comments: project 0, news 3, vacancy 5.

## 8. Translations

Two sources of text:

- **Literals in the code** are English now. They are written inline with `QString::fromUtf8("...")`.
- **Dictionary keys** (`L("key")`) come from `resources/lang/RU.json`, the site's Russian
  dictionary. An English dictionary (`EN.json`, from the site) is needed for the rest of the UI.
  Until it is copied in and loaded, keys still show Russian.


`L("key")` reads `RU.json`. If the key is missing, it returns the key, so a typo shows up on
screen instead of crashing. Text that is not in the dictionary is written inline in Russian and
marked with `// LANGXXX`. Find them with `grep -rn LANGXXX src/`.


Keys for tags exist (`Camptag1`, `Showtag1`, `Peretag1`, `Teletag1`, `Vacstag1`). Tag groups use
`tags00`–`tags03` and `os00`–`os03`.

## 9. How to add a feature

1. Pick the domain the feature belongs to (see `docs/ARCHITECTURE.md`). If none fits, create a
   folder under `src/domain/` with a `QObject` class, and add it to `ObjectHubClient.pro`.
2. Add a value to `WriteAction` in `forms.h` if the reply needs its own handling.
3. Add the request function to the domain. Tag the reply with `tagReply()` and connect `finished`
   to the domain's reply slot.
4. Add a `case` to that slot. Emit `actionFinished` on error, or a specific signal on success.
5. Declare the method on `ApiClient` in `apiclient.h` and forward it to the domain in `apiclient.cpp`.
   Forward the new signal in the `ApiClient` constructor.
6. Connect the signal in the `MainWindow` constructor and add a slot in `mainwindow.cpp`.
7. If it needs a button, create it in `setupUi()` and show it from `updateActions()`.
8. Add a check to `tests/account_integration_test.cpp` against a fake answer, then build and run it
   (see the header comment of that file).

## 10. Known gaps

- **Admin, wiki, forums, alarm push notifications:** not implemented, and not planned.
- **Comments:** after a post, the list reloads the first page and drops pages loaded earlier.
  Loading the earlier pages again would need a loop over the pages.
- **Subscription probe:** walks all pages for each project, which can be slow for big accounts.
- **Notifications:** no background refresh. Lists load when the user opens them.
- **Tags on search:** still numeric fields in the search panel. Only the edit forms use names.
- **Not compiled by the author** of the last few changes in a Qt environment. The first compile
  error is the one to fix before anything else.

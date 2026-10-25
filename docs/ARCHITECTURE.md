# Object Hub Qt client — architecture

## Idea

The UI talks only to `ApiClient`. `ApiClient` is a façade: it owns one `Session`, one `Network`,
and one object per domain. Each domain is a `QObject` that builds its requests, parses the answers,
and reports results with signals. Domains do not know about each other, and none of them touches
widgets. The state that outlives a request (the login, the owned projects, likes, subscriptions)
lives in the domain that owns it, and the UI reads it through `ApiClient`.

```
UI  (mainwindow, dialogs, resultsmodel)
          │  calls: m_api->…      signals: … Finished / … Error / … Loaded
          ▼
ApiClient  (façade, src/api/apiclient.*)          connects domain signals to its own
          │
  ┌───────┼────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
  ▼       ▼        ▼          ▼          ▼          ▼          ▼          ▼
account catalog  detail    profile    social    projects  vacancies    news    portfolio
  │       │        │                     │
  │       └──── catalogparse ───────────┘   (shared parsers for items, news, tags)
  │
  └── owns: owned projects, likes, subscriptions, saved login
          │
          ▼
core/  Session (tokens, base URL, user id) · Network (one QNetworkAccessManager, 30/120 s
       timeouts) · protocol (answer codes, error texts, [NET] logging, ALTCHA solver, multipart)
```

## Layout

| Path | Contents |
|---|---|
| `src/core/session.*` | Tokens and user id. No HTTP. |
| `src/core/network.*` | `OhNetworkAccessManager`: one NAM, auth headers, per-request timeouts. |
| `src/core/protocol.*` | Shared protocol helpers: answer error codes, `errorMessage()`, `tagReply()`, multipart parts, ALTCHA solving, `[NET]` logging. |
| `src/core/types.h` | Channel and detail codes (openGo contract). |
| `src/api/apiclient.*` | The façade. Forwards calls to domains, connects their signals, and holds the thin pieces that are not a domain (`userId`, the session-expired and account-refresh slots). |
| `src/api/forms.h`, `src/api/searchresult.h` | Plain structs: `SearchResult`, `DetailData`, `PublicProfile`, the edit forms, `WriteAction`. |
| `src/domain/account/` | Login, registration, saved login in `ObjectHub.ini`, logout, ALTCHA request, owned projects, likes, subscriptions. |
| `src/domain/catalog/` | Search, news list, vacancy list. `catalogparse` holds the item parsers shared with the detail page. |
| `src/domain/detail/` | Detail pages (project, vacancy, news, profile entries) and comment pages. |
| `src/domain/profile/` | Public profile of any user. |
| `src/domain/social/` | Votes, comment writes, own profile fields, subscriptions, notifications. |
| `src/domain/projects/` | Project edit form and save. |
| `src/domain/vacancies/` | Vacancy edit form, save, removal, applications, owner check. |
| `src/domain/news/` | News save, delete, owner check. |
| `src/domain/portfolio/` | Portfolio items: save and delete. |
| `src/domain/authoring/formparse.*` | Edit-form parsers used by projects and vacancies. |

## Rules

1. The UI calls `ApiClient` only. Nothing outside `src/api/` includes a domain header except `apiclient.cpp`.
2. A domain reports through its own signals. `ApiClient` forwards them under the same names, so
   the UI did not change when the code moved.
3. Each domain has one reply slot (`onReplyFinished`, or `onSocialReplyFinished` /
   `onAccountReplyFinished` where a domain has more than one kind of request). It reads the
   `action`, `id` and `channel` properties set by `tagReply()`.
4. Answers that change state owned by another domain are passed on as signals. Example: the
   projects domain does not reload the account itself. It emits `accountRefreshRequested()`, and
   `ApiClient` asks the account domain to reload.
5. The session is the only place that holds the token and user id. There are no copies in
   `ApiClient`.

## Qt versions

Checked: Qt 5.15 (Ubuntu 24.04), with `-Wall -Wextra` clean and the integration suite in
`tests/account_integration_test.cpp` passing. Not checked: Qt 5.6 (Windows XP build) and Qt 6.
Qt 5.0 is not verified: the app code no longer uses lambda connections (Qt 5.2+ syntax), but it still
requires C++11, and Qt 5.0 was not available here to build against.

## Migration history

1. Core: `Session`, `Network`, protocol helpers.
2. Network: per-request timeouts, no nested event loops.
3. Account domain (login, registration, restore, logout).
4. Catalog domain (search, news, vacancies) and shared item parsers.
5. Detail and profile domains.
6. Social domain (votes, comments, profile fields, subscriptions).
7. Account state moved into the account domain; authoring split into projects, vacancies, news,
   portfolio; notifications moved into social; the old write dispatcher removed; duplicate
   `models/` headers removed.

## Non-goals

- Separate processes or services.
- Changing the openGo protocol.
- Full i18n of the inline `LANGXXX` strings.

# src/domain

One folder per domain. Each domain is a `QObject` that owns its requests and parses its answers.
See `docs/ARCHITECTURE.md` for the layout and the rules.

| Folder | Domain |
|---|---|
| `account/` | login, registration, saved login, logout, owned projects, likes, subscriptions |
| `catalog/` | search, news list, vacancy list; `catalogparse` is shared with `detail/` |
| `detail/` | detail pages and comment pages |
| `profile/` | public profile |
| `social/` | votes, comment writes, profile fields, subscriptions, notifications |
| `projects/` | project edit form and save |
| `vacancies/` | vacancy edit form, save, removal, applications, owner check |
| `news/` | news save, delete, owner check |
| `portfolio/` | portfolio items |
| `authoring/` | edit-form parsers shared by projects and vacancies |

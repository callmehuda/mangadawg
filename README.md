# MangaDawg

Web baca manga + API buatan sendiri (remake gaya `febryardiansyah/manga-api`) dengan stack:

- **Akane** untuk HTTP server + static file web.
- **cpr** untuk request ke MangaDex.
- **kiseki.hpp** (di-upgrade) untuk parsing HTML metadata (OpenGraph) dari halaman detail.
- **Format response API**: YAML (`application/x-yaml`).
- **Frontend tema**: **Neo-Brutalism**.

## Endpoint API

- `GET /api`
- `GET /api/health`
- `GET /api/manga/page/:page`
- `GET /api/manga/popular/:page`
- `GET /api/recommended`
- `GET /api/manga/detail/:id`
- `GET /api/search/:query`
- `GET /api/chapter/:chapterId`

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
./build/mangadawg
```

Lalu buka: `http://127.0.0.1:8080`

## GitHub Actions

Workflow: `.github/workflows/build-run-tunnel.yml`

- Compile project.
- Jalankan server + smoke test (`/api`).
- Install cloudflared dan menjalankan tunnel.

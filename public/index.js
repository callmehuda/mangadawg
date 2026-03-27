const mangaListEl = document.getElementById('mangaList');
const detailEl = document.getElementById('detail');
const chapterImagesEl = document.getElementById('chapterImages');
const listTitleEl = document.getElementById('listTitle');

async function getYml(path) {
  const res = await fetch(path);
  const text = await res.text();
  return jsyaml.load(text);
}

function renderList(data) {
  mangaListEl.innerHTML = '';
  for (const manga of data.results || []) {
    const card = document.createElement('article');
    card.className = 'card';
    card.innerHTML = `
      <h3>${manga.title}</h3>
      <p>Status: ${manga.status}</p>
      <p>Rating: ${manga.content_rating}</p>
      <button data-id="${manga.id}">Buka Detail</button>
    `;
    card.querySelector('button').addEventListener('click', () => loadDetail(manga.id));
    mangaListEl.appendChild(card);
  }
}

async function loadLatest() {
  listTitleEl.textContent = 'Latest Manga';
  const data = await getYml('/api/manga/page/1');
  renderList(data);
}

async function loadPopular() {
  listTitleEl.textContent = 'Popular Manga';
  const data = await getYml('/api/manga/popular/1');
  renderList(data);
}

async function loadSearch() {
  const q = document.getElementById('searchInput').value.trim();
  if (!q) return;
  listTitleEl.textContent = `Hasil: ${q}`;
  const data = await getYml(`/api/search/${encodeURIComponent(q)}`);
  renderList(data);
}

async function loadDetail(id) {
  const detail = await getYml(`/api/manga/detail/${id}`);
  detailEl.innerHTML = `
    <h3>${detail.title}</h3>
    <p>${detail.description || ''}</p>
    <p>Status: ${detail.state}</p>
    <p>Genre: ${(detail.genres || []).join(', ')}</p>
    <h4>Chapters</h4>
  `;

  const list = document.createElement('div');
  for (const ch of detail.chapters || []) {
    const btn = document.createElement('button');
    btn.textContent = `Chapter ${ch.chapter} ${ch.title || ''}`;
    btn.addEventListener('click', () => loadChapter(ch.id));
    list.appendChild(btn);
  }
  detailEl.appendChild(list);
}

async function loadChapter(chapterId) {
  const chapter = await getYml(`/api/chapter/${chapterId}`);
  chapterImagesEl.innerHTML = '';
  for (const page of chapter.pages || []) {
    const img = document.createElement('img');
    img.src = page;
    img.loading = 'lazy';
    chapterImagesEl.appendChild(img);
  }
}

document.getElementById('latestBtn').addEventListener('click', loadLatest);
document.getElementById('popularBtn').addEventListener('click', loadPopular);
document.getElementById('searchBtn').addEventListener('click', loadSearch);

loadLatest();

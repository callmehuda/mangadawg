const mangaListEl = document.getElementById('mangaList');
const detailEl = document.getElementById('detail');
const chapterImagesEl = document.getElementById('chapterImages');
const listTitleEl = document.getElementById('listTitle');

async function getYml(path) {
  const response = await fetch(path);
  const raw = await response.text();
  const parsed = jsyaml.load(raw);
  if (!response.ok || parsed?.status === false) {
    throw new Error(parsed?.error || `Request gagal (${response.status})`);
  }
  return parsed;
}

function createEmptyMessage(message) {
  const el = document.createElement('p');
  el.className = 'empty';
  el.textContent = message;
  return el;
}

function renderList(data) {
  mangaListEl.innerHTML = '';
  const results = data.results || [];

  if (!results.length) {
    mangaListEl.appendChild(createEmptyMessage('Tidak ada manga ditemukan.'));
    return;
  }

  for (const manga of results) {
    const card = document.createElement('article');
    card.className = 'card';
    card.innerHTML = `
      <h3>${manga.title}</h3>
      <p>Status: ${manga.status}</p>
      <p>Tahun: ${manga.year || '-'}</p>
      <p>Rating: ${manga.content_rating}</p>
      <button data-id="${manga.id}">Buka Detail</button>
    `;
    card.querySelector('button').addEventListener('click', () => loadDetail(manga.id));
    mangaListEl.appendChild(card);
  }
}

async function renderListFromPath(path, title) {
  try {
    listTitleEl.textContent = title;
    const data = await getYml(path);
    renderList(data);
  } catch (error) {
    mangaListEl.innerHTML = '';
    mangaListEl.appendChild(createEmptyMessage(error.message));
  }
}

async function loadLatest() {
  await renderListFromPath('/api/manga/page/1', 'Latest Manga');
}

async function loadPopular() {
  await renderListFromPath('/api/manga/popular/1', 'Popular Manga');
}

async function loadRecommended() {
  await renderListFromPath('/api/recommended', 'Recommended Manga');
}

async function loadSearch() {
  const q = document.getElementById('searchInput').value.trim();
  if (!q) return;
  await renderListFromPath(`/api/search/${encodeURIComponent(q)}`, `Hasil: ${q}`);
}

async function loadDetail(id) {
  detailEl.innerHTML = 'Loading detail...';
  chapterImagesEl.innerHTML = '';

  try {
    const detail = await getYml(`/api/manga/detail/${id}`);
    detailEl.innerHTML = `
      <h3>${detail.title}</h3>
      <p>${detail.description || ''}</p>
      <p><strong>Status:</strong> ${detail.state}</p>
      <p><strong>Genre:</strong> ${(detail.genres || []).join(', ') || '-'}</p>
      <h4>Daftar Chapter (${(detail.chapters || []).length})</h4>
    `;

    const actions = document.createElement('div');
    actions.className = 'chapter-actions';

    for (const chapter of detail.chapters || []) {
      const button = document.createElement('button');
      button.textContent = `Ch ${chapter.chapter} ${chapter.title || ''}`;
      button.addEventListener('click', () => loadChapter(chapter.id));
      actions.appendChild(button);
    }

    if (!actions.childElementCount) {
      actions.appendChild(createEmptyMessage('Chapter belum tersedia.'));
    }
    detailEl.appendChild(actions);
  } catch (error) {
    detailEl.innerHTML = '';
    detailEl.appendChild(createEmptyMessage(error.message));
  }
}

async function loadChapter(chapterId) {
  chapterImagesEl.innerHTML = '<p class="empty">Loading chapter pages...</p>';
  try {
    const chapter = await getYml(`/api/chapter/${chapterId}`);
    chapterImagesEl.innerHTML = '';

    if (!(chapter.pages || []).length) {
      chapterImagesEl.appendChild(createEmptyMessage('Halaman chapter tidak tersedia di sumber upstream.'));
      return;
    }

    for (const page of chapter.pages) {
      const img = document.createElement('img');
      img.src = page;
      img.loading = 'lazy';
      img.alt = `Chapter page ${chapterId}`;
      chapterImagesEl.appendChild(img);
    }
  } catch (error) {
    chapterImagesEl.innerHTML = '';
    chapterImagesEl.appendChild(createEmptyMessage(error.message));
  }
}

document.getElementById('latestBtn').addEventListener('click', loadLatest);
document.getElementById('popularBtn').addEventListener('click', loadPopular);
document.getElementById('recommendedBtn').addEventListener('click', loadRecommended);
document.getElementById('searchBtn').addEventListener('click', loadSearch);

loadLatest();

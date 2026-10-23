const CACHE_NAME = 'chessmaster';
const OFFLINE_PAGES = new Set([
  '/',
  '/index.html',
  '/about.html',
  '/credits.html',
  '/local.html'
]);

const STATIC_ASSETS = [
  '/styles.css',
  '/font.otf',
  '/service-worker.js',
  '/manifest.json',
  '/openings.json',
  '/pieces/wk.png',
  '/pieces/wq.png',
  '/pieces/wr.png',
  '/pieces/wb.png',
  '/pieces/wn.png',
  '/pieces/wp.png',
  '/pieces/bk.png',
  '/pieces/bq.png',
  '/pieces/br.png',
  '/pieces/bb.png',
  '/pieces/bn.png',
  '/pieces/bp.png'
];

const ASSETS_TO_CACHE = [...Array.from(OFFLINE_PAGES), ...STATIC_ASSETS];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      return cache.addAll(ASSETS_TO_CACHE).catch((err) => {
        console.warn('Errore durante il caching iniziale:', err);
      });
    })
  );
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cacheName) => {
          if (cacheName !== CACHE_NAME) {
            return caches.delete(cacheName);
          }
        })
      );
    })
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  const { request } = event;
  const url = new URL(request.url);

  if (request.method !== 'GET' || !url.protocol.startsWith('http')) {
    return;
  }

  if (url.hostname === 'fonts.googleapis.com' || url.hostname === 'fonts.gstatic.com') {
    return event.respondWith(
      fetch(request).catch(() => new Response('', { status: 200 }))
    );
  }

  if (url.pathname.startsWith('/api')) {
    return event.respondWith(
      fetch(request).catch(() => new Response('Offline API Error', { status: 503 }))
    );
  }

  if (url.pathname === '/ONLINE') {
    return event.respondWith(
      fetch(request).catch(() => {
        return new Response(
          '<h1>Chess Master</h1><p>This mode requires an internet connection.</p>',
          { status: 503, headers: { 'Content-Type': 'text/html; charset=utf-8' } }
        );
      })
    );
  }

  if (OFFLINE_PAGES.has(url.pathname)) {
    return event.respondWith(
      fetch(request)
        .then((response) => {
          if (response && response.status === 200) {
            const clonedResponse = response.clone();
            caches.open(CACHE_NAME).then((cache) => {
              cache.put(request, clonedResponse);
            });
          }
          return response;
        })
        .catch(() => {
          return caches.match(request).then((cachedResponse) => {
            return cachedResponse || caches.match('/index.html') || new Response('Offline - Page Not Available', { status: 503, headers: { 'Content-Type': 'text/plain' } });
          });
        })
    );
  }

  event.respondWith(
    caches.match(request).then((cachedResponse) => {
      if (cachedResponse) {
        fetch(request).then((networkResponse) => {
          if (networkResponse && networkResponse.status === 200) {
            caches.open(CACHE_NAME).then((cache) => {
              cache.put(request, networkResponse);
            });
          }
        }).catch(() => { });
        return cachedResponse;
      }

      return fetch(request).then((response) => {
        if (response && response.status === 200) {
          const clonedResponse = response.clone();
          caches.open(CACHE_NAME).then((cache) => {
            cache.put(request, clonedResponse);
          });
        }
        return response;
      }).catch(() => {
        if (url.pathname.endsWith('.html') || !url.pathname.includes('.')) {
          return caches.match('/index.html');
        }
        return new Response('Resource Not Available Offline', { status: 503 });
      });
    })
  );
});
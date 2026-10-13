const CACHE_NAME = 'chessmaster';
const OFFLINE_PAGES = new Set([
  '/',
  '/home.html',
  '/about.html',
  '/credits.html',
  '/lan.html'
]);

const STATIC_ASSETS = [
  '/styles.css',
  '/font.otf',
  '/service-worker.js',
  '/manifest.json',
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
        console.warn('Error during caching:', err);
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

  const userAgent = request.headers.get('user-agent') || '';
  const isSafari = /safari/i.test(userAgent) && !/chrome|chromium|edg/i.test(userAgent);

  if (request.method !== 'GET') {
    return;
  }

  if (!url.protocol.startsWith('http')) {
    return;
  }

  if (url.hostname === 'fonts.googleapis.com' || url.hostname === 'fonts.gstatic.com') {
    return event.respondWith(
      fetch(request).catch(() => {
        return new Response('', { status: 200 });
      })
    );
  }

  if (url.pathname === '/service-worker.js' || url.pathname === '/manifest.json') {
    return event.respondWith(
      fetch(request).then((response) => {
        if (response && response.status === 200) {
          const clonedResponse = response.clone();
          caches.open(CACHE_NAME).then((cache) => {
            cache.put(request, clonedResponse);
          });
        }
        return response;
      }).catch(() => {
        return caches.match(request) || new Response('Resource Not Available', { status: 503 });
      })
    );
  }

  if (url.pathname.startsWith('/api') || url.pathname === '/') {
    return event.respondWith(fetch(request).catch(() => {
      return caches.match(request) || new Response('Offline', { status: 503 });
    }));
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

  if (OFFLINE_PAGES.has(url.pathname) || url.pathname === '/') {
    return event.respondWith(
      fetch(request).then((response) => {
        if (response && response.redirected) {
          return fetch(response.url);
        }

        if (response && response.status === 200) {
          const clonedResponse = response.clone();
          caches.open(CACHE_NAME).then((cache) => {
            cache.put(request, clonedResponse);
          });
        }
        return response;
      }).catch(() => {
        return caches.match(request).then((cachedResponse) => {
          if (cachedResponse) {
            return new Response(cachedResponse.body, {
              status: cachedResponse.status,
              statusText: cachedResponse.statusText,
              headers: cachedResponse.headers,
            });
          }
          return new Response('Offline - Page Not Available', {
            status: 503,
            headers: { 'Content-Type': 'text/plain' }
          });
        });
      })
    );
  }

  event.respondWith(
    fetch(request).then((response) => {
      if (response && response.status === 200) {
        const clonedResponse = response.clone();
        caches.open(CACHE_NAME).then((cache) => {
          cache.put(request, clonedResponse);
        });
      }
      return response;
    }).catch(() => {
      if (url.pathname.endsWith('.html') || !url.pathname.includes('.')) {
        return caches.match('/home.html').then((cachedResponse) => {
          if (cachedResponse) {
            return new Response(cachedResponse.body, {
              status: cachedResponse.status,
              statusText: cachedResponse.statusText,
              headers: cachedResponse.headers,
            });
          }
          return new Response('Page Not Found', { status: 404 });
        });
      }
      return caches.match(request).then((cachedAsset) => {
        return cachedAsset || new Response('Resource Not Available Offline', { status: 503 });
      });
    })
  );
});
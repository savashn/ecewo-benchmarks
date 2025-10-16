import http from 'k6/http';
import { check, sleep } from 'k6';

export let options = {
    stages: [
        { duration: '10s', target: 100 },
        { duration: '40s', target: 100 },
        { duration: '10s', target: 0 },
    ],
    thresholds: {
        http_req_duration: ['p(95)<10'],    // 95% of requests must complete below 10ms
        http_req_failed: ['rate<0.01'],     // Error rate must be less than 1%
    },
};

export default function () {
    const res = http.get('http://localhost:3000/');     // Others run on this
    // const res = http.get('http://127.0.0.1:3000/');  // Axum runs on this

    check(res, {
        'status 200': (r) => r.status === 200,
        'body is not empty': (r) => r.body.length > 0,
    });

    sleep(1); // Wait 1 second to simulate real user behavior
}

// Run with: k6 run benchmark.js

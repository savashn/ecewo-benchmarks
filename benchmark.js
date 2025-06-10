import http from 'k6/http';
import { check, sleep } from 'k6';

export let options = {
    stages: [
        { duration: '1m', target: 100 },  // Ramp up to 100 virtual users over 1 minute
        { duration: '1m', target: 100 },  // Stay at 100 users for 1 minute
        { duration: '1m', target: 0 },    // Ramp down to 0 users over 1 minute (cool-down)
    ],
    thresholds: {
        http_req_duration: ['p(95)<500'], // 95% of requests must complete below 500ms
        http_req_failed: ['rate<0.01'],   // Error rate must be less than 1%
    },
};

export default function () {
    const res = http.get('http://localhost:3000/');     // Others run at this
    // const res = http.get('http://127.0.0.1:3000/');  // Axum runs at this

    check(res, {
        'status 200': (r) => r.status === 200,
        'body is not empty': (r) => r.body.length > 0,
    });

    sleep(1); // Wait 1 second to simulate real user behavior
}

// Run with: k6 run benchmark.js
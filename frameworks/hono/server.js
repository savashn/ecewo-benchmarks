import { Hono } from 'hono';
import { serve } from '@hono/node-server';

const app = new Hono();

app.get('/', (c) => c.json({ message: 'Hello World!' }));

serve({
    fetch: app.fetch,
    port: 3000,
}, () => {
    console.log('Server is running at http://localhost:3000');
});

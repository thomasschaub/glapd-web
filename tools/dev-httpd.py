import http.server
import os.path
import subprocess

build_dir = 'build/web/apps/portable-glapd'
html_dir = 'resources/html'

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_file(os.path.join(html_dir, 'index.html'))
        elif self.path in ['/portable-glapd.js', '/portable-glapd.wasm']:
            self.cmake_build()
            path = os.path.join(build_dir, self.path[1:])
            self.send_file(path)
        else:
            path = os.path.join(html_dir, self.path[1:])
            self.send_file(path)

    def cmake_build(self):
        subprocess.run(['cmake', '--build', build_dir])

    def send_file(self, path: str):
        if not os.path.exists(path):
            print(f'{path} does not exist, sending 404')
            self.send_404()
            return

        # Send headers

        if path.endswith('.html'):
            content_type = 'text/html'
        elif path.endswith('.js'):
            content_type = 'text/javascript'
        elif path.endswith('.svg'):
            content_type = 'image/svg+xml'
        elif path.endswith('.wasm'):
            content_type = 'application/wasm'
        else:
            raise ValueError('Unknown file extension')

        self.send_response(200)
        self.send_header('Content-Type', content_type)
        self.end_headers()

        # Send body

        with open(path, 'rb') as f:
            self.wfile.write(f.read())

    def send_404(self):
        self.send_error(404)

def main():
    server_address = ('', 8000)
    server = http.server.HTTPServer(server_address, Handler)

    print(f'Starting server at http://127.0.0.1:{server_address[1]}')
    server.serve_forever()


if __name__ == '__main__':
    main()

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p client_python/storage client_cpp/storage server_cpp/storage

echo "[samples] Gerando PNG mínimo..."
cat > /tmp/pixel.png.base64 <<'B64'
iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4nGNgYAAAAAMAASsJTYQAAAAASUVORK5CYII=
B64
base64 -d /tmp/pixel.png.base64 > /tmp/pixel.png

echo "[samples] Gerando PDF mínimo..."
cat > /tmp/min.pdf.base64 <<'B64'
JVBERi0xLjQKJeLjz9MKMSAwIG9iago8PC9UeXBlL1BhZ2UvUGFyZW50IDIgMCBSL1Jlc291cmNlczw8Pj4vTWVkaWFCb3hbMCAwIDU5NSA4NDJdL0NvbnRlbnRzIDMgMCBSPj4KZW5kb2JqCjIgMCBvYmoKPDwvVHlwZS9QYWdlcy9LaWRzWzEgMCBSXS9Db3VudCAxPj4KZW5kb2JqCjMgMCBvYmoKPDwvTGVuZ3RoIDQ0Pj4Kc3RyZWFtCkJUIC9GMSAxMiBUZgovVGYgMTIgVGQKKCBEZW1vIFBERiApIFQKRVQKZW5kc3RyZWFtCmVuZG9iagogNCAwIG9iago8PC9UeXBlL0ZvbnQvU3VidHlwZS9UeXBlMS9CYXNlRm9udC9GMS9OYW1lL0YxPj4KZW5kb2JqCnhyZWYKMCA1CjAwMDAwMDAwMDAgNjU1MzUgZgowMDAwMDAwMTI1IDAwMDAwIG4KMDAwMDAwMDA3MCAwMDAwMCBuCjAwMDAwMDAxNzkgMDAwMDAgbgowMDAwMDAwMzI0IDAwMDAwIG4KdHJhaWxlcgo8PC9Sb290IDEgMCBSL1NpemUgNT4+CnN0YXJ0eHJlZgo0NjYKJSVFT0YK
B64
base64 -d /tmp/min.pdf.base64 > /tmp/min.pdf

cp -f /tmp/pixel.png client_python/storage/pixel.png
cp -f /tmp/min.pdf client_python/storage/sample.pdf
cp -f /tmp/pixel.png client_cpp/storage/pixel.png
cp -f /tmp/min.pdf client_cpp/storage/sample.pdf

echo "[samples] Concluído. Arquivos em client_python/storage e client_cpp/storage."

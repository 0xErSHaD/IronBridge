from flask import Flask, request, jsonify
import os
from cryptography.hazmat.primitives.asymmetric import padding as rsa_padding
from cryptography.hazmat.primitives.serialization import load_pem_private_key
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

try:
    with open("private_key.pem", "rb") as key_file:
        private_key = load_pem_private_key(key_file.read(), password=None)
except FileNotFoundError:
    print("[-] Error: 'private_key.pem' not found.")
    private_key = None

def decrypt_aes_cfb8(key, iv, ciphertext):
    decryptor = Cipher(algorithms.AES(key), modes.CFB8(iv)).decryptor()
    return decryptor.update(ciphertext) + decryptor.finalize()

@app.route('/upload', methods=['POST'])
def upload_file():
    if not private_key:
        return jsonify({'error': 'Missing private key'}), 500

    file_path = request.headers.get('X-File-Path')
    chunk_index = int(request.headers.get('X-Chunk-Index', 0))
    total_chunks = int(request.headers.get('X-Total-Chunks', 1))
    enc_key_hex = request.headers.get('X-Enc-Key')
    iv_hex = request.headers.get('X-IV')

    if not file_path or not enc_key_hex or not iv_hex:
        return jsonify({'error': 'Missing headers'}), 400

    try:
        enc_key_bytes = bytes.fromhex(enc_key_hex)
        iv_bytes = bytes.fromhex(iv_hex)
    except ValueError:
        return jsonify({'error': 'Invalid hex'}), 400

    try:
        aes_key = private_key.decrypt(enc_key_bytes, rsa_padding.PKCS1v15())
    except Exception as e:
        return jsonify({'error': f'RSA Decryption failed: {str(e)}'}), 403

    chunk_data = request.data
    try:
        decrypted_chunk = decrypt_aes_cfb8(aes_key, iv_bytes, chunk_data)
    except Exception as e:
        return jsonify({'error': f'AES Decryption failed: {str(e)}'}), 403

    save_path = os.path.join(UPLOAD_FOLDER, file_path)
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    mode = 'wb' if chunk_index == 0 else 'ab'
    
    with open(save_path, mode) as f:
        f.write(decrypted_chunk)

    print(f"[*] Decrypted chunk {chunk_index + 1}/{total_chunks} for '{file_path}'")
    if chunk_index + 1 == total_chunks:
        print(f"[+] File '{file_path}' fully assembled and decrypted!\n")

    return jsonify({'message': 'Success'}), 200

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000, debug=True)
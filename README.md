<div align="center">

# 🌉 IronBridge

```text
██╗██████╗  ██████╗ ███╗   ██╗██████╗ ██████╗ ██╗██████╗  ██████╗ ███████╗
██║██╔══██╗██╔═══██╗████╗  ██║██╔══██╗██╔══██╗██║██╔══██╗██╔════╝ ██╔════╝
██║██████╔╝██║   ██║██╔██╗ ██║██████╔╝██████╔╝██║██║  ██║██║  ███╗█████╗  
██║██╔══██╗██║   ██║██║╚██╗██║██╔══██╗██╔══██╗██║██║  ██║██║   ██║██╔══╝  
██║██║  ██║╚██████╔╝██║ ╚████║██████╔╝██║  ██║██║██████╔╝╚██████╔╝███████╗
╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝  ╚═════╝ ╚══════╝
```

![Language](https://img.shields.io/badge/Language-C%20%7C%20Python-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6)
![Crypto](https://img.shields.io/badge/Crypto-AES--CFB%20%2B%20RSA-brightgreen)
![Architecture](https://img.shields.io/badge/Architecture-Client--Server-orange)

### 🔒 Secure Emergency File Backup & Exfiltration Tool

*IronBridge is a high-performance Client-Server security solution designed to safely encrypt and exfiltrate critical files directly to a remote server using native Windows APIs, Memory Mapping, and Hybrid Cryptography.*

---

</div>

## ⚙️ How It Works (Execution Flow)

```mermaid
graph TD
    subgraph Client ["💻 Client Side (Windows C CLI)"]
        direction TB
        A["1. User Selects Target Path"]
        B["2. Windows API Memory Mapping<br/><i>(Fast RAM-buffered read)</i>"]
        C["3. Generate Random AES Key & IV<br/><i>(BCrypt RNG)</i>"]
        D["4. Encrypt AES Key with RSA Public Key<br/><i>(PKCS#1 v1.5 Padding)</i>"]
        E["5. Chunk & Encrypt File Data<br/><i>(AES-CFB / 10MB Chunks)</i>"]
        F["6. Send HTTP POST Requests (WinHTTP)"]

        A --> B --> C --> D --> E --> F
    end

    subgraph Server ["🌐 Server Side (Python Flask)"]
        direction TB
        G["7. Receive & Parse Headers"]
        H["8. Decrypt AES Key via RSA Private Key"]
        I["9. Decrypt Chunk (AES-CFB8)"]
        J["10. Reassemble & Save File to /uploads Directory"]

        G --> H --> I --> J
    end

    F -->|"<b>HTTP POST Headers:</b><br/>• X-File-Path<br/>• X-Enc-Key (RSA)<br/>• X-IV<br/>• X-Chunk-Index"| G

    style Client fill:#1f2937,stroke:#3b82f6,stroke-width:2px,color:#fff
    style Server fill:#1f2937,stroke:#10b981,stroke-width:2px,color:#fff
```

---

## ✨ Key Features

* **Hybrid Cryptography:** Combines fast symmetric encryption (AES-256 CFB) for file data with asymmetric encryption (RSA-2048) for secure key transfer.
* **Native Windows API:** Utilizes Windows `BCrypt` for cryptography and `WinHTTP` for network communication without external heavy C libraries.
* **Memory Mapping:** Uses `CreateFileMapping` and `MapViewOfFile` to map files directly into RAM for optimal reading performance.
* **Chunked Streaming:** Automatically splits large files into 10MB chunks to ensure stable HTTP transport.

---

## 🛠️ Build & Setup

### 1. Client (C / Windows)
Requires a C compiler on Windows with access to system libraries (`bcrypt.lib`, `winhttp.lib`).

**Using GCC (MinGW):**
```bash
cd Client
gcc src/main.c src/core.c src/ui.c -o IronBridge.exe -I include -lbcrypt -lwinhttp
```

**Using MSVC (cl.exe):**
```cmd
cd Client
cl.exe /I include src\*.c /link bcrypt.lib winhttp.lib /out:IronBridge.exe
```

### 2. Server (Python / Flask)
```bash
cd Server
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

---

## ⚠️ Important Configurations

* **Server Endpoint:** The client connects to `127.0.0.1:5000` by default. Update the host IP in `Client/src/core.c` inside the `WinHttpConnect` function if running over a network.
* **RSA Keypair:** The RSA public key is hardcoded in `Client/src/core.c` (`rsa_pub_blob`), and its matching private key must be located at `Server/private_key.pem`.

---

## 🚀 Execution & Output

**Step 1: Start the Server**
```bash
cd Server
python app.py
```
*Output:* Server listens on `http://127.0.0.1:5000`.

**Step 2: Run the Client**
```cmd
cd Client
IronBridge.exe
```

**Sample Interactive Console Output:**
```text
========================================
      IRON BRIDGE - Security Tool       
========================================
[+] Enter path (directory/file) (1)
[+] Help (2)
[+] About (3)

Enter option(1/2/3) : 1
[+] Please enter the full path to the file/directory:
> C:\Users\DedSec\Documents\Important.docx

[*] Initializing memory mapping for file: 'C:\Users\DedSec\Documents\Important.docx'
[INFO] File Size: 154820 bytes
[+] File successfully loaded into RAM!
[+] AES Key successfully encrypted with RSA!
[*] Sending Important.docx in 1 chunks...
    -> Sending chunk 1/1 (154820 bytes)...
[+] Network handles closed safely.
```

**Server Console Output:**
```text
[*] Decrypted chunk 1/1 for 'Important.docx'
[+] File 'Important.docx' fully assembled and decrypted!
```

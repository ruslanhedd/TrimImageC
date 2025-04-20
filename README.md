
---

```markdown
# 🖼️ Image Trimmer (Console + WinAPI)

A lightweight C++ **console application** that uses basic **Windows API dialogs** to let you select, crop, and save images.  
There’s no full graphical UI — only standard system dialogs and a simple interactive cropping window.

---

## ✨ Features

- Select image files using native Windows "Open File" dialog
- Preview image in a minimal resizable window
- Select crop region using mouse
- Save the cropped image via "Save File" dialog
- Supports `.jpg`, `.png`, `.bmp`, and other common formats
- Powered by:
  - `stb_image.h` (image loading)
  - `stb_image_write.h` (image saving)
  - `stb_image_resize.h` (image resizing)  
    ⚠️ Note: uses `stb_image_resize.h`, **not** `stb_image_resize2.h`

---

## 🛠️ Build Instructions

### 🧾 Requirements

- Windows
- `g++` with C++17 support (e.g., MinGW-w64)
- Headers in `include/` directory:
  - `stb_image.h`
  - `stb_image_write.h`
  - `stb_image_resize.h`

### 🧱 Build Command

```bash
g++ main.cpp -o image_trimmer_gui.exe -Iinclude -std=c++17 -Wall -O2 -lm -lcomdlg32 -static
```

This produces a statically-linked `.exe` that can run standalone on most Windows systems.

---

## 🚀 Usage

1. Run the compiled `image_trimmer_gui.exe` from the terminal or file explorer.
2. A file picker will appear – choose an image.
3. A window opens showing the image.
4. Use the mouse to drag-select the crop area.
5. After releasing the mouse, a "Save As" dialog will appear.
6. Save the cropped image to your desired location.

---

## 🗂️ Folder Structure

```
project/
├── main.cpp
├── include/
│   ├── stb_image.h
│   ├── stb_image_write.h
│   └── stb_image_resize.h
└── README.md
```

---

## 📃 License

This project uses public domain image libraries by [nothings/stb](https://github.com/nothings/stb).  
You are free to use, modify, and distribute this code without restriction.

---

## 🙏 Credits

- [Sean Barrett](https://github.com/nothings) for `stb` libraries
- Windows API documentation

---

```
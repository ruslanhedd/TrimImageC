
---

```markdown
# 🖼️ Image Trimmer (Console + WinAPI)

A lightweight C++ **console application** that uses basic **Windows API dialogs** to let you select, crop, and save images.  
There’s no full graphical UI — only standard system dialogs and a simple interactive cropping window.

---

## ✨ Features

- Select image files using native Windows "Open File" dialog
- Preview image in a minimal resizable window
- Supports `.jpg`, `.png` for sure
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

### 🛠️ How to Use

1. Run the compiled `image_trimmer_gui.exe` from the terminal or file explorer.  
2. A file picker dialog will appear — select one or more image files.  
3. The program will open each selected image one by one.  
4. For each image:  
   - A window will appear showing the image.  
   - Use the mouse to **drag-select** the crop area.  
   - Once you release the mouse, the image is cropped accordingly.  
5. The cropped version of each image is automatically saved to an `output/` folder located in the **same directory** as the original image.  
6. The file name remains the same as the original.

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
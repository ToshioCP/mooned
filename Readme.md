# Mooned

日本語の解説が英語の解説の後にあります。

### What's mooned ?

Mooned is a simple editor using GTK+3.

### Requirements

1. Linux operating system
2. gcc, make
3. Gtk+3.22

### Installation

1. Click 'Clone or Download' button, Click 'Download ZIP' in the small dialog.
2. Extract the zip file.
3. Open terminal and change its current directory to the directory extracted from the zip file.
4. Type 'make' (you don't need to type single quotes).
5. Type './mooned' then you can run mooned.

### If you want to use meson in installation

1. Extract the zip file above and change the current directory to the obtained directory.
2. Type 'meson _build'.
3. Type 'ninja -C _build'.
4. Type '_build/mooned' then you can run mooned.
5. If you want to install it into /usr/local/bin, type 'ninja -C _build install' as a root user.
6. Then, you can run it by just typing 'mooned'. 
7. For uninstall, type 'ninja -C _build uninstall' as a root user.

### licence

Copyright (C) 2017  ToshioCP (Sekiya Toshio)

Mooned is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

Mooned is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the [GNU Lesser General Public License](https://www.gnu.org/licenses/lgpl-2.1.html) for more details.

--------------------------

### moonedって何？

moonedはGTK+3を使ったシンプルなエディタです。

### 動作条件

1. Linuxオペレーティングシステム
2. gcc, make
3. Gtk+3.22

### インストール

1. 'Clone or Download'ボタンをクリックし、現れた小さなダイアログの'Download ZIP'をクリックする
2. ダウンロードしたZipファイルを解凍する
3. 端末を起動し、カレントディレクトリをzipファイルを解凍したディレクトリに移動する
4. makeとタイプする
5. ./moonedとタイプするとmoonedを起動することができます

### mesonを使ってインストールする場合

1. 上記によって入手したzipファイルを解凍し、そのディレクトリにカレントディレクトリを移動する
2. meson _build とタイプする
3. ninja -C _build とタイプする
4. _build/mooned とタイプするとmoonedを起動することができる
5. もし、/usr/local/binにインストールしたい場合は、rootユーザになり、ninja -C _build install とタイプする
6. このインストールを行った後には、単にmoonedとタイプするだけで起動できるようになる
7. アンインストールしたい場合は、rootユーザになり、ninja -C _build uninstallとタイプする

### ライセンス

Copyright (C) 2017  ToshioCP (関谷 敏雄)

moonedはフリーソフトウェアです。
moonedは、フリーソフトウェア財団によって発行されたGNU 劣等一般公衆利用許諾契約書(バージョン2.1 か、希望によってはそれ以降のバージョンのうちどれか)の定める条件の下で再頒布または改変することができます。

moonedは有用であることを願って頒布されますが、*全くの無保証* です。商業可能性の保証や特定の目的への適合性は、言外に示されたものも含め全く存在しません。
詳しくは[GNU 劣等一般公衆利用許諾契約書](https://www.gnu.org/licenses/lgpl-2.1.html)をご覧ください。
八田真行氏による[GNU 劣等一般公衆利用許諾契約書の日本語訳](https://osdn.net/projects/opensource/wiki/licenses%2FGNU_Library_or_Lesser_General_Public_License)もご覧ください。

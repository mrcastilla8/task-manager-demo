# Avisos de dependencias para el paquete Windows

ConcurrentSimulator enlaza dinámicamente Qt 6 (Qt Core, Gui y Widgets) y usa el
plugin de plataforma Windows. Qt publica los términos aplicables y los avisos de
código de terceros en su [página de licencias](https://www.qt.io/licensing/) y
en la [documentación Qt 6](https://doc.qt.io/qt-6/licensing.html). El kit puede
ofrecer componentes bajo LGPLv3, GPL u otras licencias; se debe revisar el
SBOM/aviso del kit concreto antes de distribuir una compilación.
Se incluyen copias del texto LGPL-3.0-or-later y GPL-3.0-only en esta carpeta.
Los textos se obtuvieron de la [lista SPDX de licencias](https://github.com/spdx/license-list-data/tree/main/text).

El compilador MinGW incluye runtime GCC/libstdc++ (GPLv3 con GCC Runtime
Library Exception) y componentes MinGW-w64 con avisos propios. La distribución
portable copia el directorio `licenses` de la versión exacta de MinGW usada;
véanse también el [aviso de libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html)
y los avisos MinGW incluidos en el ZIP.

El repositorio no contiene `LICENSE` para el código propio. Este documento no
otorga una licencia al proyecto; la licencia del proyecto debe definirse antes
de publicar una Release pública. La carpeta de Qt y MinGW en el ZIP debe
corresponder a las DLL efectivamente incluidas.

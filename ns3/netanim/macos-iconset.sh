mkdir NetAnim.iconset
sips -z 16 16     ./resources/netanim-logo.png --out NetAnim.iconset/icon_16x16.png
sips -z 32 32     ./resources/netanim-logo.png --out NetAnim.iconset/icon_16x16@2x.png
sips -z 32 32     ./resources/netanim-logo.png --out NetAnim.iconset/icon_32x32.png
sips -z 64 64     ./resources/netanim-logo.png --out NetAnim.iconset/icon_32x32@2x.png
sips -z 128 128   ./resources/netanim-logo.png --out NetAnim.iconset/icon_128x128.png
sips -z 256 256   ./resources/netanim-logo.png --out NetAnim.iconset/icon_128x128@2x.png
sips -z 256 256   ./resources/netanim-logo.png --out NetAnim.iconset/icon_256x256.png
sips -z 512 512   ./resources/netanim-logo.png --out NetAnim.iconset/icon_256x256@2x.png
sips -z 512 512   ./resources/netanim-logo.png --out NetAnim.iconset/icon_512x512.png
cp ./resources/netanim-logo.png NetAnim.iconset/icon_512x512@2x.png
iconutil -c icns NetAnim.iconset
rm -R NetAnim.iconset

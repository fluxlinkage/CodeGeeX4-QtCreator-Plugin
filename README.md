# CodeGeeX4-QtCreator-Plugin
支持CodeGeeX4模型的Qt Creator插件
# 简介
支持本地或网络部署的[CodeGeeX4模型](https://github.com/THUDM/CodeGeeX4)。
目前不支持连接到[CodeGeeX官网](https://codegeex.cn/)。
参考了[Qt Creator源码](https://github.com/qt-creator/qt-creator)中的[Copilot插件](https://github.com/qt-creator/qt-creator/tree/master/src/plugins/copilot)。
# 安装
## 二进制安装
1. 在release中下载和你的Qt Creator版本完全一致的插件（看运气）。
2. 关闭Qt Creator。
3. 解压缩到Qt Creator安装目录（常见路径：/opt/Qt/Tools/QtCreator/或者C:/Qt/Tools/QtCreator/）。
4. 打开Qt Creator。
## 编译源代码安装
### 需求
需要cmake、Qt开发环境、Qt Creator（确认安装时选中了Plugin Development）。
Qt Creator建议13.0及以上版本。Qt开发环境的版本需要与Qt Creator一致（Qt Creator里面“帮助”>“About Qt Creator”可以查看）。
### 步骤
以Linux为例，假设Qt和Qt Creator分别位于/opt/Qt/6.6.3/gcc_64、/opt/Qt/Tools/QtCreator目录。
1. mkdir build
2. cd build
3. cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/Qt/6.6.3/gcc_64;/opt/Qt/Tools/QtCreator" -DCMAKE_INSTALL_PREFIX=/opt/Qt/Tools/QtCreator
4. make
5. sudo make install
# 使用
首先部署好[CodeGeeX4模型](https://github.com/THUDM/CodeGeeX4)。
可以使用这个[简单服务器端](https://github.com/fluxlinkage/SimpleCodeGeeX4Server)。注意：局域网部署请加入“--listen 0.0.0.0”参数。另外，确保防火墙没有拦截服务。
在Qt Creator中选择“编辑”>“Preference”，找到“CodeGeeX4”项目，设置参数。局域网使用尤其注意修改IP设置。
设置完成后应该可以使用了。敲一段代码，停顿几秒（和你显卡性能及参数设置有关），会出现提示。
按“Tab”键接受，按“Ctrl+右”组合键接受一个单词。还有“接受一行”功能，目前只能鼠标点击，没有快捷键。

# koteiterm Makefile

# コンパイラとフラグ
CC = cc
CFLAGS = -Wall -Wextra -Werror -std=gnu11 -g -O2
CFLAGS += -Iinclude
CFLAGS += $(shell pkg-config --cflags freetype2 imlib2)

# ライブラリ依存
LDFLAGS = -lX11 -lXft -lfontconfig -lutil -lgif
LDFLAGS += $(shell pkg-config --libs imlib2)

# ディレクトリ
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = .

# ソースファイルとオブジェクトファイル
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# 実行ファイル名
TARGET = $(BINDIR)/koteiterm

# デフォルトターゲット
.PHONY: all
all: $(TARGET)

# 実行ファイルのビルド
$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "ビルド完了: $(TARGET)"

# オブジェクトファイルのコンパイル
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# ヘッダ依存関係
$(OBJECTS): $(INCDIR)/koteiterm.h $(wildcard $(SRCDIR)/*.h)
$(OBJDIR)/display.o: $(SRCDIR)/color.h
$(OBJDIR)/color.o: $(SRCDIR)/color.h

# クリーンアップ
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "クリーンアップ完了"

# 実行
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# デバッグ実行
.PHONY: debug
debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)
	gdb ./$(TARGET)

# インストール（将来実装）
.PHONY: install
install: $(TARGET)
	@echo "インストール機能は未実装です"

# テスト（将来実装）
.PHONY: test
test:
	@echo "テスト機能は未実装です"

# ヘルプ
.PHONY: help
help:
	@echo "利用可能なターゲット:"
	@echo "  all     - ビルド（デフォルト）"
	@echo "  clean   - クリーンアップ"
	@echo "  run     - ビルドして実行"
	@echo "  debug   - デバッグビルドしてgdb起動"
	@echo "  install - インストール（未実装）"
	@echo "  test    - テスト実行（未実装）"
	@echo "  help    - このヘルプを表示"

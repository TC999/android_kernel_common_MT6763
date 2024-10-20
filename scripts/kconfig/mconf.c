/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Released under the terms of the GNU GPL v2.0.
 *
 * Introduced single menu mode (show all sub-menus in one large tree).
 * 2002-11-06 Petr Baudis <pasky@ucw.cz>
 *
 * i18n, 2005, Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>

#include "lkc.h"
#include "lxdialog/dialog.h"

static const char mconf_readme[] = N_(
"总览\n"
"--------\n"
"这个界面允许您选择构建的特性和参数。\n"
"特性可以是内置的、模块化的，或者被忽略。\n"
"参数必须以十进制或十六进制数字或文本的形式输入。\n"
"\n"
"以下列括号开头的菜单项代表特性：\n"
"  [ ] 可以被内置或移除\n"
"  < > 可以被内置、模块化或移除\n"
"  { } 可以被内置或模块化（由其他特性选择）\n"
"  - - 由其他特性选择，\n"
"而括号内的 *、M 或空格分别表示要内置、作为模块构建或排除该特性。\n"
"\n"
"要更改这些特性中的任何一个，请用光标键突出显示它，\n"
"然后按 <Y> 将其内置，按 <M> 使其成为模块，或者按 <N> 将其移除。\n"
"您也可以按空格键循环浏览可用选项（即 Y->N->M->Y）。\n"
"\n"
"一些额外的键盘提示：\n"
"\n"
"菜单\n"
"----------\n"
"o 使用上下箭头键（光标键）突出显示您想要更改的项\n"
"  或者您想要选择的子菜单，然后按 <Enter>。\n"
"  子菜单由 “--->” 表示，空的由 “----” 表示。"
"\n"
"   快捷方式：按选项的高亮字母（热键）。\n"
"            多次按热键将按顺序通过所有使用该热键的可见项。\n"
"\n"
"   您也可以使用 <PAGE UP> 和 <PAGE DOWN> 键滚动\n"
"   未显示的选项进入视野。\n"
"\n"
"o  要退出菜单，请使用光标键突出显示 <Exit> 按钮\n"
"   然后按 <ENTER>。\n"
"\n"
"   快捷方式：按 <ESC><ESC> 或 <E> 或 <X> 如果没有热键\n"
"            使用这些字母。您可以按单个 <ESC>，但\n"
"            会有延迟响应，您可能会觉得烦恼。\n"
"\n"
"   另外，<TAB> 和光标键将在 <Select>、<Exit>、<Help>、<Save> 和 <Load> 之间循环。\n"
"\n"
"o  要获取某项的帮助，请使用光标键突出显示 <Help> 然后按 <ENTER>。\n"
"\n"
"   快捷方式：按 <H> 或 <?>。\n"
"\n"
"o  要切换显示隐藏选项，请按 <Z>。\n"
"\n"
"\n"
"无线电列表（选择列表）\n"
"-----------\n"
"o  使用光标键选择您想要设置的选项，然后按 <S> 或空格键。\n"
"\n"
"   快捷方式：按您想要设置的选项的第一个字母，然后按 <S> 或空格键。\n"
"\n"
"o  要查看该项的可用帮助，使用光标键突出显示<Help> 然后按 <ENTER>。\n"
"\n"
"   快捷方式：按 <H> 或 <?>。\n"
"\n"
"   另外，<TAB> 和光标键将在 <Select> 和 <Help> 之间循环\n"
"\n"
"\n"
"数据输入\n"
"-----------\n"
"o  输入所请求的信息，然后按 <ENTER> 。\n"
"   如果您正在输入十六进制值，不需要在输入前加上 '0x' 前缀。\n"
"\n"
"o  要获取帮助，请使用 <TAB> 或光标键突出显示帮助选项然后按 <ENTER>。\n"
"   您也可以尝试 <TAB><H>。\n"
"\n"
"\n"
"文本框（帮助窗口）\n"
"--------\n"
"o  使用光标键向上/向下/向左/向右滚动。VI 编辑器h、j、k、l 键在这里起作用，\n"
"   同样适用于熟悉 less 和 lynx 的人 <u>、<d>、空格键和 <B>。\n"
"\n"
"o  按 <E>、<X>、<q>、<Enter> 或 <Esc><Esc> 退出。\n"
"\n"
"\n"
"其他配置文件\n"
"-----------------------------\n"
"Menuconfig 支持使用其他配置文件对于那些出于各种原因，\n"
"发现有必要在不同配置之间切换的人。\n"
"\n"
"<Save> 按钮将允许您将当前配置保存到您选择的文件中。\n"
"使用 <Load> 按钮加载先前保存的其他配置。\n"
"\n"
"即使您不使用其他配置文件，但如果您在 Menuconfig 会话中发现\n"
"您完全搞砸了您的设置，您可以使用 <Load> 按钮恢复您之前保存的设置，\n"
"来自 “.config”，而无需重新启动 Menuconfig。\n"
"\n"
"其他信息\n"
"-----------------\n"
"如果您在 XTERM 窗口中使用 Menuconfig，请确保您的 $TERM 变量设置指向支持颜色的\n"
"xterm 定义。否则，Menuconfig 看起来会很糟糕。\n"
"Menuconfig 在 RXVT 窗口中无法正确显示，因为 rxvt 只显示一种颜色强度，亮色。\n"
"\n"
"Menuconfig 将在屏幕上或 xterms 上显示更大的菜单，这些屏幕或 xterms 设置为\n"
"显示超过标准 25 行乘以 80 列的几何形状。为了使这工作，“stty size”命令必须能够\n"
"显示屏幕的当前行和列几何形状。我强烈建议您确保您没有将 LINES 和 COLUMNS 这两个\n"
" shell 变量导出到您的环境。一些发行版通过 /etc/profile 导出这些变量。一些\n"
" ncurses 程序可能会混淆，当这些变量（LINES & COLUMNS）不反映真正的屏幕尺寸。\n"
"\n"
"可选个性可用\n"
"------------------------------\n"
"如果您希望在单个菜单中列出所有选项，而不是默认的多菜单层次结构，运行 menuconfig 时\n"
"将 MENUCONFIG_MODE 环境变量设置为 single_menu。示例：\n"
"\n"
"make MENUCONFIG_MODE=single_menu menuconfig\n"
"\n"
"<Enter> 然后将展开相应的类别，或者如果它已经展开，则将其折叠。\n"
"\n"
"请注意，这种模式最终可能会稍微增加 CPU 开销（特别是有更多展开的类别时）比默认模式。\n"
"\n"
"不同的颜色主题可用\n"
"--------------------------------\n"
"可以通过变量 MENUCONFIG_COLOR 选择不同的颜色主题。要选取主题，请使用：\n"
"\n"
"make MENUCONFIG_COLOR=<theme> menuconfig\n"
"\n"
"可用主题有\n"
" mono       => 选择适合单色显示器的颜色\n"
" blackbg    => 选择黑色背景的颜色方案\n"
" classic    => 蓝色背景的主题。经典外观\n"
" bluetitle  => 经典的 LCD 友好版本。（默认）\n"

"\n"),
menu_instructions[] = N_(
	"箭头键可以浏览菜单。\n"
	"<Enter> 选择子菜单 --->（或空的子菜单 ----）。\n"
	"高亮显示的字母是快捷键。\n"
	"按 <Y> 包含，<N> 排除，<M> 模块化特性。\n"
	"按 <Esc><Esc> 退出，<?> 寻求帮助，</> 进行搜索。\n"
	"图例：[*] 内置 [ ] 排除 <M> 模块 < > 模块化能力\n"),
radiolist_instructions[] = N_(
	"使用箭头键浏览此窗口，或者按下您想要选择的项目对应的快捷键\n"
	"然后按空格键。\n"
	"按<?>获取关于此选项的更多信息。\n"),
inputbox_instructions_int[] = N_(
	"请输入一个十进制数值。\n"
	"不接受分数。\n"
	"使用 <TAB> 键从输入字段移动到下面的按钮。\n"),
inputbox_instructions_hex[] = N_(
	"请输入十六进制值。\n"
	"使用 <TAB> 键从输入字段移动到下面的按钮。\n"),
inputbox_instructions_string[] = N_(
	"请输入字符串。\n"
	"使用 <TAB> 键从输入字段移动到下面的按钮。\n"),
setmod_text[] = N_(
	"该功能取决于另一个已配置为模块的功能。\n"
	"因此，该功能将作为一个模块构建。"),
load_config_text[] = N_(
	"输入要加载的配置文件的名称。 \n"
	"接受显示的名称，以还原上次获取的配置。留空表示放弃。\n"),
load_config_help[] = N_(
	"\n"
	"由于各种原因，人们可能希望在一台机器上保留几种不同的配置。\n"
	"\n"
	"如果您之前将配置保存在非默认文件中，在这里输入其名称将允许您修改该配置。\n"
	"\n"
	"如果您不确定，那么您可能从未使用过其他配置文件。因此，您应该留空以中止。\n"),
save_config_text[] = N_(
	"输入应将此配置保存为备用配置的文件名。 留空表示放弃。\n"),
save_config_help[] = N_(
	"\n"
	"由于各种原因，人们可能希望在一台机器上保留不同的配置。\n"
	"\n"
	"在这里输入一个文件名，将允许您以后检索、修改并使用当前的配置作为\n"
	"您当时所选择的配置选项的替代方案。\n"
	"\n"
	"如果您不确定这全部意味着什么，那么您应该留空。\n"),
search_help[] = N_(
	"\n"
	"搜索符号并显示它们的关系。\n"
	" 允许使用正则表达式。\n"
	"示例：搜索\"^FOO\"\n"
	"结果：\n"
	"-----------------------------------------------------------------\n"
	"Symbol: FOO [=m]\n"
	"Type  : tristate\n"
	"Prompt: Foo bus is used to drive the bar HW\n"
	"  Location:\n"
	"    -> Bus options (PCI, PCMCIA, EISA, ISA)\n"
	"      -> PCI support (PCI [=y])\n"
	"(1)     -> PCI access mode (<choice> [=y])\n"
	"  Defined at drivers/pci/Kconfig:47\n"
	"  Depends on: X86_LOCAL_APIC && X86_IO_APIC || IA64\n"
	"  Selects: LIBCRC32\n"
	"  Selected by: BAR [=n]\n"
	"-----------------------------------------------------------------\n"
	"o The line 'Type:' shows the type of the configuration option for\n"
	"  this symbol (boolean, tristate, string, ...)\n"
	"o The line 'Prompt:' shows the text used in the menu structure for\n"
	"  this symbol\n"
	"o The 'Defined at' line tells at what file / line number the symbol\n"
	"  is defined\n"
	"o The 'Depends on:' line tells what symbols need to be defined for\n"
	"  this symbol to be visible in the menu (selectable)\n"
	"o The 'Location:' lines tells where in the menu structure this symbol\n"
	"  is located\n"
	"    A location followed by a [=y] indicates that this is a\n"
	"    selectable menu item - and the current value is displayed inside\n"
	"    brackets.\n"
	"    Press the key in the (#) prefix to jump directly to that\n"
	"    location. You will be returned to the current search results\n"
	"    after exiting this new menu.\n"
	"o The 'Selects:' line tells what symbols will be automatically\n"
	"  selected if this symbol is selected (y or m)\n"
	"o The 'Selected by' line tells what symbol has selected this symbol\n"
	"\n"
	"Only relevant lines are shown.\n"
	"\n\n"
	"Search examples:\n"
	"Examples: USB	=> find all symbols containing USB\n"
	"          ^USB => find all symbols starting with USB\n"
	"          USB$ => find all symbols ending with USB\n"
	"\n");

static int indent;
static struct menu *current_menu;
static int child_count;
static int single_menu_mode;
static int show_all_options;
static int save_and_exit;
static int silent;

static void conf(struct menu *menu, struct menu *active_menu);
static void conf_choice(struct menu *menu);
static void conf_string(struct menu *menu);
static void conf_load(void);
static void conf_save(void);
static int show_textbox_ext(const char *title, char *text, int r, int c,
			    int *keys, int *vscroll, int *hscroll,
			    update_text_fn update_text, void *data);
static void show_textbox(const char *title, const char *text, int r, int c);
static void show_helptext(const char *title, const char *text);
static void show_help(struct menu *menu);

static char filename[PATH_MAX+1];
static void set_config_filename(const char *config_filename)
{
	static char menu_backtitle[PATH_MAX+128];
	int size;

	size = snprintf(menu_backtitle, sizeof(menu_backtitle),
			"%s - %s", config_filename, rootmenu.prompt->text);
	if (size >= sizeof(menu_backtitle))
		menu_backtitle[sizeof(menu_backtitle)-1] = '\0';
	set_dialog_backtitle(menu_backtitle);

	size = snprintf(filename, sizeof(filename), "%s", config_filename);
	if (size >= sizeof(filename))
		filename[sizeof(filename)-1] = '\0';
}

struct subtitle_part {
	struct list_head entries;
	const char *text;
};
static LIST_HEAD(trail);

static struct subtitle_list *subtitles;
static void set_subtitle(void)
{
	struct subtitle_part *sp;
	struct subtitle_list *pos, *tmp;

	for (pos = subtitles; pos != NULL; pos = tmp) {
		tmp = pos->next;
		free(pos);
	}

	subtitles = NULL;
	list_for_each_entry(sp, &trail, entries) {
		if (sp->text) {
			if (pos) {
				pos->next = xcalloc(1, sizeof(*pos));
				pos = pos->next;
			} else {
				subtitles = pos = xcalloc(1, sizeof(*pos));
			}
			pos->text = sp->text;
		}
	}

	set_dialog_subtitles(subtitles);
}

static void reset_subtitle(void)
{
	struct subtitle_list *pos, *tmp;

	for (pos = subtitles; pos != NULL; pos = tmp) {
		tmp = pos->next;
		free(pos);
	}
	subtitles = NULL;
	set_dialog_subtitles(subtitles);
}

struct search_data {
	struct list_head *head;
	struct menu **targets;
	int *keys;
};

static void update_text(char *buf, size_t start, size_t end, void *_data)
{
	struct search_data *data = _data;
	struct jump_key *pos;
	int k = 0;

	list_for_each_entry(pos, data->head, entries) {
		if (pos->offset >= start && pos->offset < end) {
			char header[4];

			if (k < JUMP_NB) {
				int key = '0' + (pos->index % JUMP_NB) + 1;

				sprintf(header, "(%c)", key);
				data->keys[k] = key;
				data->targets[k] = pos->target;
				k++;
			} else {
				sprintf(header, "   ");
			}

			memcpy(buf + pos->offset, header, sizeof(header) - 1);
		}
	}
	data->keys[k] = 0;
}

static void search_conf(void)
{
	struct symbol **sym_arr;
	struct gstr res;
	struct gstr title;
	char *dialog_input;
	int dres, vscroll = 0, hscroll = 0;
	bool again;
	struct gstr sttext;
	struct subtitle_part stpart;

	title = str_new();
	str_printf( &title, _("Enter (sub)string or regexp to search for "
			      "(with or without \"%s\")"), CONFIG_);

again:
	dialog_clear();
	dres = dialog_inputbox(_("Search Configuration Parameter"),
			      str_get(&title),
			      10, 75, "");
	switch (dres) {
	case 0:
		break;
	case 1:
		show_helptext(_("Search Configuration"), search_help);
		goto again;
	default:
		str_free(&title);
		return;
	}

	/* strip the prefix if necessary */
	dialog_input = dialog_input_result;
	if (strncasecmp(dialog_input_result, CONFIG_, strlen(CONFIG_)) == 0)
		dialog_input += strlen(CONFIG_);

	sttext = str_new();
	str_printf(&sttext, "Search (%s)", dialog_input_result);
	stpart.text = str_get(&sttext);
	list_add_tail(&stpart.entries, &trail);

	sym_arr = sym_re_search(dialog_input);
	do {
		LIST_HEAD(head);
		struct menu *targets[JUMP_NB];
		int keys[JUMP_NB + 1], i;
		struct search_data data = {
			.head = &head,
			.targets = targets,
			.keys = keys,
		};
		struct jump_key *pos, *tmp;

		res = get_relations_str(sym_arr, &head);
		set_subtitle();
		dres = show_textbox_ext(_("Search Results"), (char *)
					str_get(&res), 0, 0, keys, &vscroll,
					&hscroll, &update_text, (void *)
					&data);
		again = false;
		for (i = 0; i < JUMP_NB && keys[i]; i++)
			if (dres == keys[i]) {
				conf(targets[i]->parent, targets[i]);
				again = true;
			}
		str_free(&res);
		list_for_each_entry_safe(pos, tmp, &head, entries)
			free(pos);
	} while (again);
	free(sym_arr);
	str_free(&title);
	list_del(trail.prev);
	str_free(&sttext);
}

static void build_conf(struct menu *menu)
{
	struct symbol *sym;
	struct property *prop;
	struct menu *child;
	int type, tmp, doint = 2;
	tristate val;
	char ch;
	bool visible;

	/*
	 * note: menu_is_visible() has side effect that it will
	 * recalc the value of the symbol.
	 */
	visible = menu_is_visible(menu);
	if (show_all_options && !menu_has_prompt(menu))
		return;
	else if (!show_all_options && !visible)
		return;

	sym = menu->sym;
	prop = menu->prompt;
	if (!sym) {
		if (prop && menu != current_menu) {
			const char *prompt = menu_get_prompt(menu);
			switch (prop->type) {
			case P_MENU:
				child_count++;
				prompt = _(prompt);
				if (single_menu_mode) {
					item_make("%s%*c%s",
						  menu->data ? "-->" : "++>",
						  indent + 1, ' ', prompt);
				} else
					item_make("   %*c%s  %s",
						  indent + 1, ' ', prompt,
						  menu_is_empty(menu) ? "----" : "--->");
				item_set_tag('m');
				item_set_data(menu);
				if (single_menu_mode && menu->data)
					goto conf_childs;
				return;
			case P_COMMENT:
				if (prompt) {
					child_count++;
					item_make("   %*c*** %s ***", indent + 1, ' ', _(prompt));
					item_set_tag(':');
					item_set_data(menu);
				}
				break;
			default:
				if (prompt) {
					child_count++;
					item_make("---%*c%s", indent + 1, ' ', _(prompt));
					item_set_tag(':');
					item_set_data(menu);
				}
			}
		} else
			doint = 0;
		goto conf_childs;
	}

	type = sym_get_type(sym);
	if (sym_is_choice(sym)) {
		struct symbol *def_sym = sym_get_choice_value(sym);
		struct menu *def_menu = NULL;

		child_count++;
		for (child = menu->list; child; child = child->next) {
			if (menu_is_visible(child) && child->sym == def_sym)
				def_menu = child;
		}

		val = sym_get_tristate_value(sym);
		if (sym_is_changable(sym)) {
			switch (type) {
			case S_BOOLEAN:
				item_make("[%c]", val == no ? ' ' : '*');
				break;
			case S_TRISTATE:
				switch (val) {
				case yes: ch = '*'; break;
				case mod: ch = 'M'; break;
				default:  ch = ' '; break;
				}
				item_make("<%c>", ch);
				break;
			}
			item_set_tag('t');
			item_set_data(menu);
		} else {
			item_make("   ");
			item_set_tag(def_menu ? 't' : ':');
			item_set_data(menu);
		}

		item_add_str("%*c%s", indent + 1, ' ', _(menu_get_prompt(menu)));
		if (val == yes) {
			if (def_menu) {
				item_add_str(" (%s)", _(menu_get_prompt(def_menu)));
				item_add_str("  --->");
				if (def_menu->list) {
					indent += 2;
					build_conf(def_menu);
					indent -= 2;
				}
			}
			return;
		}
	} else {
		if (menu == current_menu) {
			item_make("---%*c%s", indent + 1, ' ', _(menu_get_prompt(menu)));
			item_set_tag(':');
			item_set_data(menu);
			goto conf_childs;
		}
		child_count++;
		val = sym_get_tristate_value(sym);
		if (sym_is_choice_value(sym) && val == yes) {
			item_make("   ");
			item_set_tag(':');
			item_set_data(menu);
		} else {
			switch (type) {
			case S_BOOLEAN:
				if (sym_is_changable(sym))
					item_make("[%c]", val == no ? ' ' : '*');
				else
					item_make("-%c-", val == no ? ' ' : '*');
				item_set_tag('t');
				item_set_data(menu);
				break;
			case S_TRISTATE:
				switch (val) {
				case yes: ch = '*'; break;
				case mod: ch = 'M'; break;
				default:  ch = ' '; break;
				}
				if (sym_is_changable(sym)) {
					if (sym->rev_dep.tri == mod)
						item_make("{%c}", ch);
					else
						item_make("<%c>", ch);
				} else
					item_make("-%c-", ch);
				item_set_tag('t');
				item_set_data(menu);
				break;
			default:
				tmp = 2 + strlen(sym_get_string_value(sym)); /* () = 2 */
				item_make("(%s)", sym_get_string_value(sym));
				tmp = indent - tmp + 4;
				if (tmp < 0)
					tmp = 0;
				item_add_str("%*c%s%s", tmp, ' ', _(menu_get_prompt(menu)),
					     (sym_has_value(sym) || !sym_is_changable(sym)) ?
					     "" : _(" (NEW)"));
				item_set_tag('s');
				item_set_data(menu);
				goto conf_childs;
			}
		}
		item_add_str("%*c%s%s", indent + 1, ' ', _(menu_get_prompt(menu)),
			  (sym_has_value(sym) || !sym_is_changable(sym)) ?
			  "" : _(" (NEW)"));
		if (menu->prompt->type == P_MENU) {
			item_add_str("  %s", menu_is_empty(menu) ? "----" : "--->");
			return;
		}
	}

conf_childs:
	indent += doint;
	for (child = menu->list; child; child = child->next)
		build_conf(child);
	indent -= doint;
}

static void conf(struct menu *menu, struct menu *active_menu)
{
	struct menu *submenu;
	const char *prompt = menu_get_prompt(menu);
	struct subtitle_part stpart;
	struct symbol *sym;
	int res;
	int s_scroll = 0;

	if (menu != &rootmenu)
		stpart.text = menu_get_prompt(menu);
	else
		stpart.text = NULL;
	list_add_tail(&stpart.entries, &trail);

	while (1) {
		item_reset();
		current_menu = menu;
		build_conf(menu);
		if (!child_count)
			break;
		set_subtitle();
		dialog_clear();
		res = dialog_menu(prompt ? _(prompt) : _("Main Menu"),
				  _(menu_instructions),
				  active_menu, &s_scroll);
		if (res == 1 || res == KEY_ESC || res == -ERRDISPLAYTOOSMALL)
			break;
		if (item_count() != 0) {
			if (!item_activate_selected())
				continue;
			if (!item_tag())
				continue;
		}
		submenu = item_data();
		active_menu = item_data();
		if (submenu)
			sym = submenu->sym;
		else
			sym = NULL;

		switch (res) {
		case 0:
			switch (item_tag()) {
			case 'm':
				if (single_menu_mode)
					submenu->data = (void *) (long) !submenu->data;
				else
					conf(submenu, NULL);
				break;
			case 't':
				if (sym_is_choice(sym) && sym_get_tristate_value(sym) == yes)
					conf_choice(submenu);
				else if (submenu->prompt->type == P_MENU)
					conf(submenu, NULL);
				break;
			case 's':
				conf_string(submenu);
				break;
			}
			break;
		case 2:
			if (sym)
				show_help(submenu);
			else {
				reset_subtitle();
				show_helptext(_("README"), _(mconf_readme));
			}
			break;
		case 3:
			reset_subtitle();
			conf_save();
			break;
		case 4:
			reset_subtitle();
			conf_load();
			break;
		case 5:
			if (item_is_tag('t')) {
				if (sym_set_tristate_value(sym, yes))
					break;
				if (sym_set_tristate_value(sym, mod))
					show_textbox(NULL, setmod_text, 6, 74);
			}
			break;
		case 6:
			if (item_is_tag('t'))
				sym_set_tristate_value(sym, no);
			break;
		case 7:
			if (item_is_tag('t'))
				sym_set_tristate_value(sym, mod);
			break;
		case 8:
			if (item_is_tag('t'))
				sym_toggle_tristate_value(sym);
			else if (item_is_tag('m'))
				conf(submenu, NULL);
			break;
		case 9:
			search_conf();
			break;
		case 10:
			show_all_options = !show_all_options;
			break;
		}
	}

	list_del(trail.prev);
}

static int show_textbox_ext(const char *title, char *text, int r, int c, int
			    *keys, int *vscroll, int *hscroll, update_text_fn
			    update_text, void *data)
{
	dialog_clear();
	return dialog_textbox(title, text, r, c, keys, vscroll, hscroll,
			      update_text, data);
}

static void show_textbox(const char *title, const char *text, int r, int c)
{
	show_textbox_ext(title, (char *) text, r, c, (int []) {0}, NULL, NULL,
			 NULL, NULL);
}

static void show_helptext(const char *title, const char *text)
{
	show_textbox(title, text, 0, 0);
}

static void conf_message_callback(const char *fmt, va_list ap)
{
	char buf[PATH_MAX+1];

	vsnprintf(buf, sizeof(buf), fmt, ap);
	if (save_and_exit) {
		if (!silent)
			printf("%s", buf);
	} else {
		show_textbox(NULL, buf, 6, 60);
	}
}

static void show_help(struct menu *menu)
{
	struct gstr help = str_new();

	help.max_width = getmaxx(stdscr) - 10;
	menu_get_ext_help(menu, &help);

	show_helptext(_(menu_get_prompt(menu)), str_get(&help));
	str_free(&help);
}

static void conf_choice(struct menu *menu)
{
	const char *prompt = _(menu_get_prompt(menu));
	struct menu *child;
	struct symbol *active;

	active = sym_get_choice_value(menu->sym);
	while (1) {
		int res;
		int selected;
		item_reset();

		current_menu = menu;
		for (child = menu->list; child; child = child->next) {
			if (!menu_is_visible(child))
				continue;
			if (child->sym)
				item_make("%s", _(menu_get_prompt(child)));
			else {
				item_make("*** %s ***", _(menu_get_prompt(child)));
				item_set_tag(':');
			}
			item_set_data(child);
			if (child->sym == active)
				item_set_selected(1);
			if (child->sym == sym_get_choice_value(menu->sym))
				item_set_tag('X');
		}
		dialog_clear();
		res = dialog_checklist(prompt ? _(prompt) : _("Main Menu"),
					_(radiolist_instructions),
					MENUBOX_HEIGTH_MIN,
					MENUBOX_WIDTH_MIN,
					CHECKLIST_HEIGTH_MIN);
		selected = item_activate_selected();
		switch (res) {
		case 0:
			if (selected) {
				child = item_data();
				if (!child->sym)
					break;

				sym_set_tristate_value(child->sym, yes);
			}
			return;
		case 1:
			if (selected) {
				child = item_data();
				show_help(child);
				active = child->sym;
			} else
				show_help(menu);
			break;
		case KEY_ESC:
			return;
		case -ERRDISPLAYTOOSMALL:
			return;
		}
	}
}

static void conf_string(struct menu *menu)
{
	const char *prompt = menu_get_prompt(menu);

	while (1) {
		int res;
		const char *heading;

		switch (sym_get_type(menu->sym)) {
		case S_INT:
			heading = _(inputbox_instructions_int);
			break;
		case S_HEX:
			heading = _(inputbox_instructions_hex);
			break;
		case S_STRING:
			heading = _(inputbox_instructions_string);
			break;
		default:
			heading = _("Internal mconf error!");
		}
		dialog_clear();
		res = dialog_inputbox(prompt ? _(prompt) : _("Main Menu"),
				      heading, 10, 75,
				      sym_get_string_value(menu->sym));
		switch (res) {
		case 0:
			if (sym_set_string_value(menu->sym, dialog_input_result))
				return;
			show_textbox(NULL, _("You have made an invalid entry."), 5, 43);
			break;
		case 1:
			show_help(menu);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static void conf_load(void)
{

	while (1) {
		int res;
		dialog_clear();
		res = dialog_inputbox(NULL, load_config_text,
				      11, 55, filename);
		switch(res) {
		case 0:
			if (!dialog_input_result[0])
				return;
			if (!conf_read(dialog_input_result)) {
				set_config_filename(dialog_input_result);
				sym_set_change_count(1);
				return;
			}
			show_textbox(NULL, _("File does not exist!"), 5, 38);
			break;
		case 1:
			show_helptext(_("Load Alternate Configuration"), load_config_help);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static void conf_save(void)
{
	while (1) {
		int res;
		dialog_clear();
		res = dialog_inputbox(NULL, save_config_text,
				      11, 55, filename);
		switch(res) {
		case 0:
			if (!dialog_input_result[0])
				return;
			if (!conf_write(dialog_input_result)) {
				set_config_filename(dialog_input_result);
				return;
			}
			show_textbox(NULL, _("Can't create file!  Probably a nonexistent directory."), 5, 60);
			break;
		case 1:
			show_helptext(_("Save Alternate Configuration"), save_config_help);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static int handle_exit(void)
{
	int res;

	save_and_exit = 1;
	reset_subtitle();
	dialog_clear();
	if (conf_get_changed())
		res = dialog_yesno(NULL,
				   _("Do you wish to save your new configuration?\n"
				     "(Press <ESC><ESC> to continue kernel configuration.)"),
				   6, 60);
	else
		res = -1;

	end_dialog(saved_x, saved_y);

	switch (res) {
	case 0:
		if (conf_write(filename)) {
			fprintf(stderr, _("\n\n"
					  "Error while writing of the configuration.\n"
					  "Your configuration changes were NOT saved."
					  "\n\n"));
			return 1;
		}
		/* fall through */
	case -1:
		if (!silent)
			printf(_("\n\n"
				 "*** End of the configuration.\n"
				 "*** Execute 'make' to start the build or try 'make help'."
				 "\n\n"));
		res = 0;
		break;
	default:
		if (!silent)
			fprintf(stderr, _("\n\n"
					  "Your configuration changes were NOT saved."
					  "\n\n"));
		if (res != KEY_ESC)
			res = 0;
	}

	return res;
}

static void sig_handler(int signo)
{
	exit(handle_exit());
}

int main(int ac, char **av)
{
	char *mode;
	int res;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	signal(SIGINT, sig_handler);

	if (ac > 1 && strcmp(av[1], "-s") == 0) {
		silent = 1;
		/* Silence conf_read() until the real callback is set up */
		conf_set_message_callback(NULL);
		av++;
	}
	conf_parse(av[1]);
	conf_read(NULL);

	mode = getenv("MENUCONFIG_MODE");
	if (mode) {
		if (!strcasecmp(mode, "single_menu"))
			single_menu_mode = 1;
	}

	if (init_dialog(NULL)) {
		fprintf(stderr, N_("Your display is too small to run Menuconfig!\n"));
		fprintf(stderr, N_("It must be at least 19 lines by 80 columns.\n"));
		return 1;
	}

	set_config_filename(conf_get_configname());
	conf_set_message_callback(conf_message_callback);
	do {
		conf(&rootmenu, NULL);
		res = handle_exit();
	} while (res == KEY_ESC);

	return res;
}

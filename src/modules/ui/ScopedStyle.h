/**
 * @file
 */

#pragma once

#include "IMGUIApp.h"
#include "core/Color.h"

namespace ui {

class ScopedStyle {
private:
	int _n = 0;
	int _font = 0;
	int _color = 0;

public:
	~ScopedStyle() {
		ImGui::PopStyleVar(_n);
		ImGui::PopStyleColor(_color);
		resetFontSize();
	}
	inline void setColor(ImGuiCol idx, const ImVec4 &col) {
		ImGui::PushStyleColor(idx, col);
		++_color;
	}
	inline void resetColors(int n = 1) {
		ImGui::PopStyleColor(n);
		_color -= n;
	}
	inline void darker(ImGuiCol idx, float f = 1.0f) {
		setColor(idx, core::Color::darker(ImGui::GetStyle().Colors[idx], f));
	}
	inline void brighter(ImGuiCol idx, float f = 1.0f) {
		setColor(idx, core::Color::brighter(ImGui::GetStyle().Colors[idx], f));
	}
	/**
	 * Does either make the color brighter or darker depending on the current color
	 */
	inline void highlight(ImGuiCol idx, float f = 1.0f) {
		const glm::vec4 &c = ImGui::GetStyle().Colors[idx];
		if (c.x < 0.1f && c.y < 0.1f && c.z < 0.1f) {
			setColor(idx, core::Color::brighter(c, f));
			return;
		}
		setColor(idx, core::Color::darker(c, f));
	}
	inline void pushFontSize(int size) {
		ImGui::PushFont(imguiApp()->defaultFont(), size);
		++_font;
	}
	inline void resetFontSize() {
		if (_font > 0) {
			ImGui::PopFont();
			--_font;
		}
	}
	inline void setAlpha(float alpha) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		++_n;
	}
	inline void setDisabledAlpha(float alpha) {
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, alpha);
		++_n;
	}
	inline void setWindowRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, val);
		++_n;
	}
	inline void setWindowBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, val);
		++_n;
	}
	inline void setChildRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, val);
		++_n;
	}
	inline void setChildBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, val);
		++_n;
	}
	inline void setPopupRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, val);
		++_n;
	}
	inline void setPopupBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, val);
		++_n;
	}
	inline void setFrameRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, val);
		++_n;
	}
	inline void setFrameBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, val);
		++_n;
	}
	inline void setIndentSpacing(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, val);
		++_n;
	}
	inline void setScrollbarSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, val);
		++_n;
	}
	inline void setScrollbarRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, val);
		++_n;
	}
	inline void setGrabMinSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, val);
		++_n;
	}
	inline void setGrabRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, val);
		++_n;
	}
	inline void setTabRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, val);
		++_n;
	}
	inline void setWindowPadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, val);
		++_n;
	}
	inline void setWindowMinSize(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, val);
		++_n;
	}
	inline void setWindowTitleAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, val);
		++_n;
	}
	inline void setFramePadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, val);
		++_n;
	}
	inline void setItemSpacing(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, val);
		++_n;
	}
	inline void setItemInnerSpacing(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, val);
		++_n;
	}
	inline void setCellPadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, val);
		++_n;
	}
	inline void setButtonTextAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, val);
		++_n;
	}
	inline void setSelectableTextAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, val);
		++_n;
	}
};

class ScopedStyleCompact : public ScopedStyle {
public:
	ScopedStyleCompact() {
	    ImGuiStyle& style = ImGui::GetStyle();
		setFramePadding(ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
		setItemSpacing(ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
	}
};

} // namespace ui

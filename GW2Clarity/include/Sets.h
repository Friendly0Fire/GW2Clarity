#pragma once

#include <Main.h>
#include <Graphics.h>
#include <glm/glm.hpp>
#include <map>
#include <ConfigurationFile.h>
#include <mutex>
#include <span>
#include <imgui.h>
#include <set>
#include <ActivationKeybind.h>
#include <SettingsMenu.h>

namespace GW2Clarity
{

class Grids;

class Sets : public SettingsMenu::Implementer
{
public:
	Sets(ComPtr<ID3D11Device>& dev, Grids* grids);
	virtual ~Sets();

	void Draw(ComPtr<ID3D11DeviceContext>& ctx);
	void DrawMenu(Keybind** currentEditedKeybind) override;

	const char* GetTabName() const override { return "Sets"; }

	void Delete(short id);
	void GridDeleted(Id id);

protected:
	void Load(size_t gridCount);
	void Save();

public:
	struct Set
	{
		std::string name;
		std::set<int> grids;
		bool combatOnly = false;
	};
	
	const std::vector<Set>& sets() const { return sets_; }

	short currentSetId() const { return currentSetId_; }
	const Set* currentSet() const { return currentSetId_ >= 0 && currentSetId_ < sets_.size() ? &sets_[currentSetId_] : nullptr; }
	bool enableDefaultSet() const { return sets_.empty(); }

protected:
	Set creatingSet_;
	short currentSetId_ = UnselectedSubId;
	short currentHoveredSet_ = UnselectedSubId;

	Grids* gridsInstance_ = nullptr;
	
	static inline const char* ChangeSetPopupName = "QuickSet";
	
	std::vector<Set> sets_;
	
	short selectedSetId_ = UnselectedSubId;
	mstime lastSaveTime_ = 0;
	bool needsSaving_ = false;
	static inline constexpr mstime SaveDelay = 1000;
	bool showSetSelector_ = false;
	bool firstDraw_ = true;

	ActivationKeybind changeGridSetKey_;
};
}
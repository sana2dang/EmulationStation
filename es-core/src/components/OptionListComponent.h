#pragma once
#ifndef ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H
#define ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H

#include "GuiComponent.h"
#include "Log.h"
#include "Window.h"
#include "EsLocale.h"

//Used to display a list of options.
//Can select one or multiple options.

// if !multiSelect
// * <- curEntry ->

// always
// * press a -> open full list

#define CHECKED_PATH ":/checkbox_checked.svg"
#define UNCHECKED_PATH ":/checkbox_unchecked.svg"

template<typename T>
class OptionListComponent : public GuiComponent
{
private:
	struct OptionListData
	{
		std::string name;
		T object;
		bool selected;
	};

	class OptionListPopup : public GuiComponent
	{
	private:
		MenuComponent mMenu;
		OptionListComponent<T>* mParent;

	public:
		OptionListPopup(Window* window, OptionListComponent<T>* parent, const std::string& title, 
			const std::function<void(T& data, ComponentListRow& row)> callback = nullptr) : GuiComponent(window),
			mMenu(window, title.c_str()), mParent(parent)
		{
			auto menuTheme = ThemeData::getMenuTheme();
			auto font = menuTheme->Text.font;
			auto color = menuTheme->Text.color;
			
			ComponentListRow row;

			// for select all/none
			std::vector<ImageComponent*> checkboxes;

			for(auto it = mParent->mEntries.begin(); it != mParent->mEntries.end(); it++)
			{
				row.elements.clear();

				OptionListData& e = *it;

				if (callback != nullptr)
				{
					callback(e.object, row);

					if (!mParent->mMultiSelect)
					{
						row.makeAcceptInputHandler([this, &e]
						{
							e.selected = !e.selected;
							mParent->onSelectedChanged();
						});
					}
					else
					{
						row.makeAcceptInputHandler([this, &e]
						{
							mParent->mEntries.at(mParent->getSelectedId()).selected = false;
							e.selected = true;
							mParent->onSelectedChanged();
							delete this;
						});
					}
				}
				else
				{
					row.addElement(std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(it->name), font, color), true);

					if (mParent->mMultiSelect)
					{
						// add checkbox
						auto checkbox = std::make_shared<ImageComponent>(mWindow);
						checkbox->setImage(it->selected ? CHECKED_PATH : UNCHECKED_PATH);
						checkbox->setResize(0, font->getLetterHeight());
						row.addElement(checkbox, false);

						// input handler
						// update checkbox state & selected value
						row.makeAcceptInputHandler([this, &e, checkbox]
						{
							e.selected = !e.selected;
							checkbox->setImage(e.selected ? CHECKED_PATH : UNCHECKED_PATH);
							mParent->onSelectedChanged();
						});

						// for select all/none
						checkboxes.push_back(checkbox.get());
					}
					else {
						// input handler for non-multiselect
						// update selected value and close
						row.makeAcceptInputHandler([this, &e]
						{
							mParent->mEntries.at(mParent->getSelectedId()).selected = false;
							e.selected = true;
							mParent->onSelectedChanged();
							delete this;
						});
					}
				}

				// also set cursor to this row if we're not multi-select and this row is selected
				mMenu.addRow(row, (!mParent->mMultiSelect && it->selected));
			}

			mMenu.addButton(_("BACK"), _("accept"), [this] { delete this; });

			if(mParent->mMultiSelect)
			{
				mMenu.addButton(_("SELECT ALL"), _("SELECT ALL"), [this, checkboxes] {
					for(unsigned int i = 0; i < mParent->mEntries.size(); i++)
					{
						mParent->mEntries.at(i).selected = true;
						checkboxes.at(i)->setImage(CHECKED_PATH);
					}
					mParent->onSelectedChanged();
				});

				mMenu.addButton(_("SELECT NONE"), _("SELECT NONE"), [this, checkboxes] {
					for(unsigned int i = 0; i < mParent->mEntries.size(); i++)
					{
						mParent->mEntries.at(i).selected = false;
						checkboxes.at(i)->setImage(UNCHECKED_PATH);
					}
					mParent->onSelectedChanged();
				});
			}

			mMenu.setPosition(
				(Renderer::getScreenWidth() - mMenu.getSize().x()) / 2,
				(Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
				//Renderer::getScreenHeight() * 0.15f);
			addChild(&mMenu);
		}

		bool input(InputConfig* config, Input input) override
		{
			if(config->isMappedTo("b", input) && input.value != 0)
			{
				delete this;
				return true;
			}

			return GuiComponent::input(config, input);
		}

		std::vector<HelpPrompt> getHelpPrompts() override
		{
			auto prompts = mMenu.getHelpPrompts();
			prompts.push_back(HelpPrompt("b", _("BACK")));
			return prompts;
		}
	};

public:
	OptionListComponent(Window* window, const std::string& name, bool multiSelect = false) : GuiComponent(window), mMultiSelect(multiSelect), mName(name),
		 mText(window), mLeftArrow(window), mRightArrow(window)
	{
		auto theme = ThemeData::getMenuTheme();
		
		mAddRowCallback = nullptr;

		mText.setFont(theme->Text.font);
		mText.setColor(theme->Text.color);
		mText.setHorizontalAlignment(ALIGN_CENTER);
		addChild(&mText);

		mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
		mRightArrow.setResize(0, mText.getFont()->getLetterHeight());

		if (mMultiSelect)
		{
			mRightArrow.setImage(ThemeData::getMenuTheme()->Icons.arrow);// ":/arrow.svg");
			mRightArrow.setColorShift(theme->Text.color);
			addChild(&mRightArrow);
		} else {
			mLeftArrow.setImage(ThemeData::getMenuTheme()->Icons.option_arrow); //  ":/option_arrow.svg"
			mLeftArrow.setColorShift(theme->Text.color);
			mLeftArrow.setFlipX(true);
			addChild(&mLeftArrow);

			mRightArrow.setImage(ThemeData::getMenuTheme()->Icons.option_arrow); // ":/option_arrow.svg");
			mRightArrow.setColorShift(theme->Text.color);
			addChild(&mRightArrow);
		}

		setSize(mLeftArrow.getSize().x() + mRightArrow.getSize().x(), theme->Text.font->getHeight());
	}


	virtual void setColor(unsigned int color)
	{
		mText.setColor(color);
		mLeftArrow.setColorShift(color);
		mRightArrow.setColorShift(color);		
	}

	// handles positioning/resizing of text and arrows
	void onSizeChanged() override
	{
		mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
		mRightArrow.setResize(0, mText.getFont()->getLetterHeight());

		if(mSize.x() < (mLeftArrow.getSize().x() + mRightArrow.getSize().x()))
			LOG(LogWarning) << "OptionListComponent too narrow!";

		mText.setSize(mSize.x() - mLeftArrow.getSize().x() - mRightArrow.getSize().x(), mText.getFont()->getHeight());

		// position
		mLeftArrow.setPosition(0, (mSize.y() - mLeftArrow.getSize().y()) / 2);
		mText.setPosition(mLeftArrow.getPosition().x() + mLeftArrow.getSize().x(), (mSize.y() - mText.getSize().y()) / 2);
		mRightArrow.setPosition(mText.getPosition().x() + mText.getSize().x(), (mSize.y() - mRightArrow.getSize().y()) / 2);
	}

	bool input(InputConfig* config, Input input) override
	{
		if(input.value != 0)
		{
			if(config->isMappedTo("a", input))
			{
				open();
				return true;
			}
			if(!mMultiSelect)
			{
				if(config->isMappedLike("left", input))
				{
					// move selection to previous
					unsigned int i = getSelectedId();
					int next = (int)i - 1;
					if(next < 0)
						next += (int)mEntries.size();

					mEntries.at(i).selected = false;
					mEntries.at(next).selected = true;
					onSelectedChanged();
					return true;

				}else if(config->isMappedLike("right", input))
				{
					if (mEntries.size() == 0)
						return true;

					// move selection to next
					unsigned int i = getSelectedId();
					int next = (i + 1) % mEntries.size();
					mEntries.at(i).selected = false;
					mEntries.at(next).selected = true;
					onSelectedChanged();
					return true;

				}
			}
		}
		return GuiComponent::input(config, input);
	}

	std::vector<T> getSelectedObjects()
	{
		std::vector<T> ret;
		for(auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
		{
			if(it->selected)
				ret.push_back(it->object);
		}

		return ret;
	}

	T getSelected()
	{
		assert(mMultiSelect == false);
		auto selected = getSelectedObjects();
		assert(selected.size() == 1);
		return selected.at(0);
	}

	void add(const std::string& name, const T& obj, bool selected)
	{
		OptionListData e;
		e.name = name;
		e.object = obj;
		e.selected = selected;

		mEntries.push_back(e);
		onSelectedChanged();
	}

	void selectAll()
	{
		for(unsigned int i = 0; i < mEntries.size(); i++)
		{
			mEntries.at(i).selected = true;
		}
		onSelectedChanged();
	}

	void selectNone()
	{
		for(unsigned int i = 0; i < mEntries.size(); i++)
		{
			mEntries.at(i).selected = false;
		}
		onSelectedChanged();
	}

	void selectFirstItem()
	{
		for (unsigned int i = 0; i < mEntries.size(); i++)
			mEntries.at(i).selected = false;

		if (mEntries.size() > 0)
			mEntries.at(0).selected = true;

		onSelectedChanged();
	}

	void clear() {
		mEntries.clear();
	}

	inline void invalidate() {
		onSelectedChanged();
	}

	void setSelectedChangedCallback(const std::function<void(const T&)>& callback) 
	{
		mSelectedChangedCallback = callback;
	}

	void setRowTemplate(std::function<void(T& data, ComponentListRow& row)> callback)
	{
		mAddRowCallback = callback;
	}

private:
	std::function<void(T& data, ComponentListRow& row)> mAddRowCallback;

	void open()
	{
		mWindow->pushGui(new OptionListPopup(mWindow, this, mName, mAddRowCallback));
	}

	unsigned int getSelectedId()
	{
		assert(mMultiSelect == false);
		for(unsigned int i = 0; i < mEntries.size(); i++)
		{
			if(mEntries.at(i).selected)
				return i;
		}

		LOG(LogWarning) << "OptionListComponent::getSelectedId() - no selected element found, defaulting to 0";
		return 0;
	}

	void onSelectedChanged()
	{
		if(mMultiSelect)
		{
			// display # selected

			

			char strbuf[256];
			int x = getSelectedObjects().size();
			snprintf(strbuf, 256, EsLocale::nGetText("%i SELECTED", "%i SELECTED", x).c_str(), x);
			mText.setText(strbuf);


			mText.setSize(0, mText.getSize().y());
			setSize(mText.getSize().x() + mRightArrow.getSize().x() + 24, mText.getSize().y());
			if(mParent) // hack since theres no "on child size changed" callback atm...
				mParent->onSizeChanged();
		}else{
			// display currently selected + l/r cursors
			for(auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
			{
				if (it->selected)
				{
					mText.setText(Utils::String::toUpper(it->name));
					mText.setSize(0, mText.getSize().y());
					setSize(mText.getSize().x() + mLeftArrow.getSize().x() + mRightArrow.getSize().x() + 24, mText.getSize().y());
					if (mParent) // hack since theres no "on child size changed" callback atm...
						mParent->onSizeChanged();
					break;
				}
			}
		}

		if (mSelectedChangedCallback)
			mSelectedChangedCallback(mEntries.at(getSelectedId()).object);		
	}

	std::vector<HelpPrompt> getHelpPrompts() override
	{
		std::vector<HelpPrompt> prompts;
		if(!mMultiSelect)
			prompts.push_back(HelpPrompt("left/right", "MODIFIER"));

		prompts.push_back(HelpPrompt("a", "SELECTIONNER"));
		return prompts;
	}

	bool mMultiSelect;

	std::string mName;
	TextComponent mText;
	ImageComponent mLeftArrow;
	ImageComponent mRightArrow;

	std::vector<OptionListData> mEntries;
	std::function<void(const T&)> mSelectedChangedCallback;
};

#endif // ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H

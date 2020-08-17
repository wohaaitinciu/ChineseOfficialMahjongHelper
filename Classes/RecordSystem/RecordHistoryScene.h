#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

struct Record;

struct RecordTexts {
    const Record *source;
    std::string title;
    std::string time;
    std::string players[4];
    int seats[4];
};

struct FilterIndex {
    size_t real_index;
    uint8_t player_flag;
};

class RecordHistoryScene : public BaseScene, cw::TableViewDelegate {
public:
    typedef std::function<void (Record *)> ViewCallback;

    CREATE_FUNC_WITH_PARAM_1(RecordHistoryScene, ViewCallback &&, viewCallback);
    bool init(ViewCallback &&viewCallback);

    static void modifyRecord(const Record *record);

    virtual void onEnter() override;

private:
    std::vector<RecordTexts> _recordTexts;

    struct FilterCriteria {
        time_t start_time;
        time_t finish_time;
        bool ignore_case;
        bool whole_word;
        bool regular_enabled;
        char name[64];
        char title[128];
    };
    FilterCriteria _filterCriteria;
    std::vector<FilterIndex> _filterIndices;

    void filter();
    void refresh();
    void updateRecordTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onMoreButton(cocos2d::Ref *sender);
    void showFilterAlert();
    void switchToSummary();
    void switchToBatchDelete();
    void showTransmissionAlert();

    void showSendAlert(std::vector<bool> selectFlags);
    void showRecvAlert();

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellClicked(cocos2d::Ref *sender);

    void saveRecordsAndRefresh();

    cocos2d::Label *_emptyLabel = nullptr;
    cocos2d::ui::Button *_moreButton = nullptr;
    cw::TableView *_tableView = nullptr;
    ViewCallback _viewCallback;
};

#endif

#ifndef __HAND_TILES_WIDGETS_H__
#define __HAND_TILES_WIDGETS_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/tile.h"

class HandTilesWidget : public cocos2d::Node {
public:
    CREATE_FUNC(HandTilesWidget);

    static cocos2d::Node *createStaticNode(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile);

    void setCurrentIdxChangedCallback(std::function<void ()> &&callback) { _currentIdxChangedCallback.swap(callback); }
    void setTileClickCallback(std::function<void ()> &&callback) { _tileClickCallback.swap(callback); }

    mahjong::tile_t getCurrentTile() const { return _currentIdx < _standingTiles.size() ? _standingTiles[_currentIdx] : mahjong::tile_t(0); }
    int getUsedTileCount(mahjong::tile_t tile) const { return _usedTilesTable[tile]; }

    void setData(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile);
    void getData(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *servingTile) const;

    mahjong::tile_t getServingTile() const;
    bool isFixedPacksContainsKong() const;
    bool isStandingTilesContainsServingTile() const;
    size_t countServingTileInFixedPacks() const;

    void discardCurrentTile();

    virtual bool init() override;

public:
    void reset();
    void sortStandingTiles();
    mahjong::tile_t putTile(mahjong::tile_t tile);

    bool canChow(int meldedIdx) const;
    bool canPung() const;
    bool canDirectKong() const;
    bool canPromotedKong() const;

    bool makeFixedChowPack(int meldedIdx);
    bool makeFixedPungPack();
    bool makeFixedMeldedKongPack();
    bool makeFixedConcealedKongPack();

private:
    cocos2d::Node *_fixedContainer = nullptr;
    cocos2d::Node *_standingContainer = nullptr;
    std::vector<cocos2d::ui::Button *> _standingTileButtons;
    cocos2d::ui::Widget *_emptyTileWidget = nullptr;
    cocos2d::DrawNode *_highlightBox = nullptr;

    mahjong::tile_table_t _usedTilesTable;
    mahjong::tile_table_t _standingTilesTable;
    std::vector<mahjong::tile_t> _standingTiles;
    std::vector<mahjong::pack_t> _fixedPacks;
    size_t _currentIdx = 0;

    std::function<void ()> _currentIdxChangedCallback;
    std::function<void ()> _tileClickCallback;

private:
    void addTile(mahjong::tile_t tile);
    void replaceTile(mahjong::tile_t tile);
    void refreshEmptyWidgetPos();
    void refreshHighlightPos();
    void refreshStandingTiles();
    void refreshStandingTilesPos();

    void onEmptyWidget(cocos2d::Ref *sender);
    void onTileButton(cocos2d::Ref *sender);

    int calcMeldedIdx(int maxIdx) const;
    void addFixedChowPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedPungPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedMeldedKongPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedConcealedKongPack(mahjong::tile_t tile);
    void promoteFixedPungPackToKongPack(mahjong::tile_t tile, size_t idx);

    bool makeFixedDircetMeldedKongPack();
    bool promoteFixedPungPack();
};

#endif

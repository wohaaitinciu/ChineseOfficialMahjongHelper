#include "HandTilesWidget.h"
#include <algorithm>
#include <iterator>
#include "../mahjong-algorithm/fan_calculator.h"
#include "../utils/compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

#define GAP 4

#define MAX_STANDING_TILES_COUNT(fixedPackSize_) (13 - (fixedPackSize_) * 3)

bool HandTilesWidget::init() {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

    // 14张牌宽度：TILE_WIDTH * 14
    Size standingSize = Size(TILE_WIDTH * 14 + GAP, TILE_HEIGHT);
    Node *node = Node::create();
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    node->setIgnoreAnchorPointForPosition(false);
    node->setContentSize(standingSize);
    this->addChild(node);
    node->setPosition(Vec2(standingSize.width * 0.5f, standingSize.height * 0.5f));
    _standingContainer = node;

    ui::Widget *widget = ui::Widget::create();
    widget->setTouchEnabled(true);
    widget->setContentSize(Size(Size(TILE_WIDTH, TILE_HEIGHT)));
    _standingContainer->addChild(widget);
    widget->addClickEventListener(std::bind(&HandTilesWidget::onEmptyWidget, this, std::placeholders::_1));
    _emptyTileWidget = widget;
    widget->addChild(LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x20), TILE_WIDTH, TILE_HEIGHT));

    // 高亮方框
    DrawNode *drawNode = DrawNode::create();
    drawNode->setContentSize(Size(TILE_WIDTH, TILE_HEIGHT));
    drawNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    drawNode->setLineWidth(3.0f);
    drawNode->drawRect(Vec2(0.0f, 0.0f), Vec2(TILE_WIDTH, TILE_HEIGHT), Color4F::RED);
    drawNode->drawSolidRect(Vec2(0.0f, 0.0f), Vec2(TILE_WIDTH, TILE_HEIGHT), Color4F(0.05f, 0.05f, 0.05f, 0.1f));
    _standingContainer->addChild(drawNode, 2);
    _highlightBox = drawNode;

    const float fixedHeight = std::max<float>(TILE_HEIGHT, TILE_WIDTH * 2);
    node = Node::create();
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    node->setIgnoreAnchorPointForPosition(false);
    node->setContentSize(Size(0, fixedHeight));
    this->addChild(node);
    node->setPosition(Vec2(standingSize.width * 0.5f, standingSize.height + fixedHeight * 0.5f + GAP));
    _fixedContainer = node;

    this->setContentSize(Size(standingSize.width, standingSize.height + fixedHeight + GAP));

    _standingTiles.reserve(14);
    _fixedPacks.reserve(4);

    reset();

    return true;
}

void HandTilesWidget::reset() {
    // 清空数据
    memset(_usedTilesTable, 0, sizeof(_usedTilesTable));
    memset(_standingTilesTable, 0, sizeof(_standingTilesTable));
    _currentIdx = 0;
    _standingTiles.clear();
    _fixedPacks.clear();

    // 清除之前的残留的立牌
    while (!_standingTileButtons.empty()) {
        _standingContainer->removeChild(_standingTileButtons.back());
        _standingTileButtons.pop_back();
    }
    _standingContainer->setContentSize(Size(TILE_WIDTH * 14 + GAP, TILE_HEIGHT));
    _emptyTileWidget->setPosition(Vec2(TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f));
    _highlightBox->setPosition(Vec2(TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f));

    // 清除之前残留的副露
    _fixedContainer->removeAllChildren();
    _fixedContainer->setContentSize(Size(0, _fixedContainer->getContentSize().height));
    _fixedContainer->setScale(1);
}

void HandTilesWidget::setData(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile) {
    reset();

    // 添加副露
    for (long i = 0; i < handTiles.pack_count; ++i) {
        mahjong::pack_t pack = handTiles.fixed_packs[i];
        _fixedPacks.push_back(pack);
        mahjong::tile_t tile = mahjong::pack_get_tile(pack);
        switch (mahjong::pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
            switch (mahjong::pack_get_offer(pack)) {
            default: addFixedChowPack(static_cast<mahjong::tile_t>(tile - 1), 0); break;
            case 2: addFixedChowPack(tile, 1); break;
            case 3: addFixedChowPack(static_cast<mahjong::tile_t>(tile + 1), 2); break;
            }
            ++_usedTilesTable[tile - 1];
            ++_usedTilesTable[tile];
            ++_usedTilesTable[tile + 1];
            break;
        case PACK_TYPE_PUNG:
            switch (mahjong::pack_get_offer(pack)) {
            default: addFixedPungPack(tile, 0); break;
            case 2: addFixedPungPack(tile, 1); break;
            case 3: addFixedPungPack(tile, 2); break;
            }
            _usedTilesTable[tile] += 3;
            break;
        case PACK_TYPE_KONG:
            if (!mahjong::is_promoted_kong(pack)) {
                switch (mahjong::pack_get_offer(pack)) {
                case 0: addFixedConcealedKongPack(tile); break;
                default: addFixedMeldedKongPack(tile, 0); break;
                case 2: addFixedMeldedKongPack(tile, 1); break;
                case 3: addFixedMeldedKongPack(tile, 3); break;
                }
            }
            else {
                switch (mahjong::pack_get_offer(pack)) {
                case 0: addFixedConcealedKongPack(tile); break;
                default: addFixedPungPack(tile, 0); promoteFixedPungPackToKongPack(tile, i); break;
                case 2: addFixedPungPack(tile, 1); promoteFixedPungPackToKongPack(tile, i); break;
                case 3: addFixedPungPack(tile, 2); promoteFixedPungPackToKongPack(tile, i); break;
                }
            }
            _usedTilesTable[tile] += 4;
            break;
        default:
            break;
        }
    }

    refreshStandingTilesPos();

    // 添加立牌
    for (long i = 0; i < handTiles.tile_count; ++i) {
        mahjong::tile_t tile = handTiles.standing_tiles[i];
        addTile(tile);
        ++_usedTilesTable[tile];
    }

    if (servingTile != 0) {
        addTile(servingTile);
        ++_usedTilesTable[servingTile];
    }

    refreshEmptyWidgetPos();
    refreshHighlightPos();
}

void HandTilesWidget::getData(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *servingTile) const {
    // 获取上的牌
    mahjong::tile_t st = getServingTile();
    *servingTile = st;

    // 获取副露
    handTiles->pack_count = static_cast<intptr_t>(std::copy(_fixedPacks.begin(), _fixedPacks.end(), std::begin(handTiles->fixed_packs))
        - std::begin(handTiles->fixed_packs));

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    // 获取立牌
    size_t cnt = std::copy(_standingTiles.begin(), _standingTiles.end() - (st == 0 ? 0 : 1), std::begin(handTiles->standing_tiles))
            - std::begin(handTiles->standing_tiles);
    handTiles->tile_count = std::min(maxCnt, cnt);
}

mahjong::tile_t HandTilesWidget::getServingTile() const {
    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_standingTiles.size() < maxCnt + 1) {
        return 0;
    }
    return _standingTiles.back();
}

bool HandTilesWidget::isFixedPacksContainsKong() const {
    return mahjong::is_fixed_packs_contains_kong(_fixedPacks.data(), _fixedPacks.size());
}

bool HandTilesWidget::isStandingTilesContainsServingTile() const {
    mahjong::tile_t servingTile = getServingTile();
    if (servingTile == 0) {
        return false;
    }
    return mahjong::is_standing_tiles_contains_win_tile(
        _standingTiles.data(), _standingTiles.size() - 1, servingTile);
}

size_t HandTilesWidget::countServingTileInFixedPacks() const {
    mahjong::tile_t servingTile = getServingTile();
    if (servingTile == 0 || _fixedPacks.empty()) {
        return 0;
    }

    return mahjong::count_win_tile_in_fixed_packs(
        _fixedPacks.data(), _fixedPacks.size(), servingTile);
}

void HandTilesWidget::onEmptyWidget(cocos2d::Ref *) {
    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    size_t cnt = _standingTiles.size();
    size_t idx = std::min(cnt, maxCnt);
    if (_currentIdx != idx) {
        _currentIdx = idx;
        refreshHighlightPos();
    }
}

void HandTilesWidget::onTileButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());
    if (_currentIdx != idx) {
        _currentIdx = idx;
        refreshHighlightPos();
    }

    if (_tileClickCallback) {
        _tileClickCallback();
    }
}

// 添加一张牌
void HandTilesWidget::addTile(mahjong::tile_t tile) {
    ui::Button *button = ui::Button::create(tilesImageName[tile]);
    button->setScale(CC_CONTENT_SCALE_FACTOR());
    _standingContainer->addChild(button);

    size_t tilesCnt = _standingTiles.size();
    button->setUserData(reinterpret_cast<void *>(tilesCnt));
    button->setTag(tile);
    button->addClickEventListener(std::bind(&HandTilesWidget::onTileButton, this, std::placeholders::_1));

    const float posY = TILE_HEIGHT * 0.5f;

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (LIKELY(tilesCnt < maxCnt)) {
        button->setPosition(Vec2(TILE_WIDTH * (tilesCnt + 0.5f), posY));
        _currentIdx = tilesCnt + 1;
    }
    else {
        button->setPosition(Vec2(TILE_WIDTH * (maxCnt + 0.5f) + GAP, posY));
    }
    ++_standingTilesTable[tile];

    _standingTileButtons.push_back(button);
    _standingTiles.push_back(tile);
}

// 替换一张牌
void HandTilesWidget::replaceTile(mahjong::tile_t tile) {
    // 直接换button的图
    ui::Button *button = _standingTileButtons[_currentIdx];
    button->loadTextureNormal(tilesImageName[tile]);
    button->setTag(tile);
    button->setVisible(true);

    mahjong::tile_t prevTile = _standingTiles[_currentIdx];
    --_standingTilesTable[prevTile];

    _standingTiles[_currentIdx] = tile;
    ++_standingTilesTable[tile];
}

mahjong::tile_t HandTilesWidget::putTile(mahjong::tile_t tile) {
    mahjong::tile_t prevTile = 0;
    if (_currentIdx >= _standingTiles.size()) {  // 新增牌
        addTile(tile);
        ++_usedTilesTable[tile];

        refreshEmptyWidgetPos();
    }
    else {  // 修改牌
        prevTile = _standingTiles[_currentIdx];  // 此位置之前的牌
        if (prevTile != tile) {  // 新选的牌与之前的牌不同，更新相关信息
            replaceTile(tile);
            --_usedTilesTable[prevTile];
            ++_usedTilesTable[tile];
        }

        // 以下代码，如果修改牌时需要增加下标，请启用
#if 1
        // 根据需要增加下标
        size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
        if (_currentIdx < maxCnt) {
            ++_currentIdx;
        }
#endif
    }

    refreshHighlightPos();
    return prevTile;
}

// 刷新空白Widget位置
void HandTilesWidget::refreshEmptyWidgetPos() {
    const float posY = TILE_HEIGHT * 0.5f;

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    size_t cnt = _standingTileButtons.size();
    if (LIKELY(cnt < maxCnt)) {
        _emptyTileWidget->setPosition(Vec2(TILE_WIDTH * (cnt + 0.5f), posY));
    }
    else {
        _emptyTileWidget->setPosition(Vec2(TILE_WIDTH * (maxCnt + 0.5f) + GAP, posY));
    }
}

// 刷新高亮位置
void HandTilesWidget::refreshHighlightPos() {
    const float posY = TILE_HEIGHT * 0.5f;

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (LIKELY(_currentIdx < maxCnt)) {
        _highlightBox->setPosition(Vec2(TILE_WIDTH * (_currentIdx + 0.5f), posY));
    }
    else {
        _highlightBox->setPosition(Vec2(TILE_WIDTH * (maxCnt + 0.5f) + GAP, posY));
    }

    if (_currentIdxChangedCallback) {
        _currentIdxChangedCallback();
    }
}

// 刷新立牌
void HandTilesWidget::refreshStandingTiles() {
    // 遍历立牌
    for (size_t i = 0, cnt = _standingTileButtons.size(); i < cnt; ) {
        ui::Button *button = _standingTileButtons[i];
        mahjong::tile_t tile = static_cast<mahjong::tile_t>(button->getTag());
        if (i >= _standingTiles.size() || tile != _standingTiles[i]) {  // 已经被删除的
            _standingContainer->removeChild(button);
            _standingTileButtons.erase(_standingTileButtons.begin() + i);
            --cnt;
        }
        else {
            ++i;
        }
    }

    //assert(_standingTiles.size() == _standingTileButtons.size());

    refreshStandingTilesPos();

    // 更新currentIdx
    size_t prevIndex = _currentIdx;
    _currentIdx = _standingTiles.size();
    if (prevIndex < _currentIdx) {
        _currentIdx = prevIndex;
    }

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_currentIdx > maxCnt) {
        _currentIdx = maxCnt;
    }

    refreshEmptyWidgetPos();
    refreshHighlightPos();
}

// 刷新立牌位置
void HandTilesWidget::refreshStandingTilesPos() {
    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    _standingContainer->setContentSize(Size(static_cast<float>(TILE_WIDTH * (maxCnt + 1) + GAP), TILE_HEIGHT));

    const float posY = TILE_HEIGHT * 0.5f;

    // 重新设置UserData及位置
    for (size_t i = 0, cnt = _standingTileButtons.size(); i < cnt; ++i) {
        ui::Button *button = _standingTileButtons[i];
        button->setUserData(reinterpret_cast<void *>(i));

        if (LIKELY(i < maxCnt)) {
            button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), posY));
        }
        else {
            button->setPosition(Vec2(TILE_WIDTH * (maxCnt + 0.5f) + GAP, posY));
        }
    }
}

// 排序立牌
void HandTilesWidget::sortStandingTiles() {
    if (UNLIKELY(_standingTiles.empty())) {
        return;
    }

    // 最后一张不参与排序
    const size_t offset = getServingTile() == 0 ? 0 : 1;

    std::sort(_standingTiles.begin(), _standingTiles.end() - offset);
    std::sort(_standingTileButtons.begin(), _standingTileButtons.end() - offset, [](ui::Button *btn1, ui::Button *btn2) {
        return btn1->getTag() < btn2->getTag();
    });

    refreshStandingTilesPos();

    if (_currentIdxChangedCallback) {
        _currentIdxChangedCallback();
    }
}

// 添加一组明顺
void HandTilesWidget::addFixedChowPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 2;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    const char *image[3];
    switch (meldedIdx) {
    default:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile + 1];
        image[2] = tilesImageName[tile + 2];
        break;
    case 1:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile - 1];
        image[2] = tilesImageName[tile + 1];
        break;
    case 2:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile - 2];
        image[2] = tilesImageName[tile - 1];
        break;
    }

    Vec2 pos[3];
    pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
    pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
    pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == 0) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组明刻
void HandTilesWidget::addFixedPungPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 2;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    Vec2 pos[3];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        break;
    case 1:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        break;
    case 2:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        break;
    }

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tile]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组明杠
void HandTilesWidget::addFixedMeldedKongPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 3;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    Vec2 pos[4];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        break;
    case 1:
    case 2:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        break;
    case 3:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_WIDTH * 3 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        break;
    }

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tile]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组暗杠
void HandTilesWidget::addFixedConcealedKongPack(mahjong::tile_t tile) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_WIDTH * 4;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    const char *image[4];
    image[0] = "tiles/bg.png";
    image[1] = tilesImageName[tile];
    image[2] = tilesImageName[tile];
    image[3] = "tiles/bg.png";

    Vec2 pos[4];
    pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
    pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
    pos[2] = Vec2(startX + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
    pos[3] = Vec2(startX + TILE_WIDTH * 3.5f, TILE_HEIGHT * 0.5f);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
    }
}

// 加杠
void HandTilesWidget::promoteFixedPungPackToKongPack(mahjong::tile_t tile, size_t idx) {
    float startX = 0.0f;
    for (size_t i = 0; i < idx; ++i) {
        mahjong::pack_t pack = _fixedPacks[i];
        switch (mahjong::pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
        case PACK_TYPE_PUNG:
            startX += TILE_HEIGHT + TILE_WIDTH * 2;
            break;
        case PACK_TYPE_KONG:
            if (mahjong::is_pack_melded(pack)) {
                startX += TILE_HEIGHT + TILE_WIDTH * (mahjong::is_promoted_kong(pack) ? 2 : 3);
            }
            else {
                startX += TILE_WIDTH * 4;
            }
            break;
        default:
            break;
        }

        startX += GAP;
    }

    uint8_t meldedIdx = mahjong::pack_get_offer(_fixedPacks[idx]) - 1;
    startX += meldedIdx * TILE_WIDTH;

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    Sprite *sprite = Sprite::create(tilesImageName[tile]);
    sprite->setScale(contentScaleFactor);
    _fixedContainer->addChild(sprite);
    sprite->setPosition(Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 1.5f));
    sprite->setRotation(-90);
}

bool HandTilesWidget::canChow(int meldedIdx) const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    // meldedIdx == 0: _XX 23吃1 tile, tile+1, tile+2
    // meldedIdx == 1: X_X 13吃2 tile-1, tile, tile+1
    // meldedIdx == 2: XX_ 12吃3 tile-2, tile-1, tile
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (!mahjong::is_honor(tile)
        && _standingTilesTable[tile - meldedIdx] > 0
        && _standingTilesTable[tile - meldedIdx + 1] > 0
        && _standingTilesTable[tile - meldedIdx + 2] > 0);
}

bool HandTilesWidget::canPung() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (_standingTilesTable[tile] >= 3);
}

bool HandTilesWidget::canDirectKong() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (_standingTilesTable[tile] >= 4);
}

bool HandTilesWidget::canPromotedKong() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return std::any_of(_fixedPacks.begin(), _fixedPacks.end(), [tile](mahjong::pack_t pack) {
        return (mahjong::pack_get_type(pack) == PACK_TYPE_PUNG && mahjong::pack_get_tile(pack) == tile);
    });
}

bool HandTilesWidget::makeFixedChowPack(int meldedIdx) {
    if (UNLIKELY(!canChow(meldedIdx))) {
        return false;
    }

    // meldedIdx == 0: _XX 23吃1 tile+1
    // meldedIdx == 1: X_X 13吃2 tile+0
    // meldedIdx == 2: XX_ 12吃3 tile-1
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(static_cast<uint8_t>(meldedIdx + 1), PACK_TYPE_CHOW, static_cast<mahjong::tile_t>(tile + 1 - meldedIdx));
    _fixedPacks.push_back(pack);

    // 这里迭代器不能连续使用，因为立牌不一定有序
    std::vector<mahjong::tile_t>::iterator it;
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - meldedIdx);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - meldedIdx + 1);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - meldedIdx + 2);
    _standingTiles.erase(it);
    --_standingTilesTable[tile - meldedIdx];
    --_standingTilesTable[tile - meldedIdx + 1];
    --_standingTilesTable[tile - meldedIdx + 2];

    addFixedChowPack(tile, meldedIdx);
    refreshStandingTiles();
    return true;
}

int HandTilesWidget::calcMeldedIdx(int maxIdx) const {
    mahjong::tile_t tile = _standingTiles[_currentIdx];

    size_t offset = 0;
    size_t maxCnt = MAX_STANDING_TILES_COUNT(_fixedPacks.size());  // 立牌数最大值（不包括和牌）
    if (UNLIKELY(_currentIdx == maxCnt)) {  // 不允许对和牌张进行副露
        offset = 1;
    }

    bool leftExists = std::any_of(_standingTiles.begin(), _standingTiles.begin() + _currentIdx,
        [tile](mahjong::tile_t t) { return tile == t; });
    bool rightExists = std::any_of(_standingTiles.begin() + _currentIdx + 1, _standingTiles.end() - offset,
        [tile](mahjong::tile_t t) { return tile == t; });
    return leftExists ? (rightExists ? 1 : maxIdx) : 0;
}

bool HandTilesWidget::makeFixedPungPack() {
    if (UNLIKELY(!canPung())) {  // 当前位置没有牌
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    int meldedIdx = calcMeldedIdx(2);
    mahjong::pack_t pack = mahjong::make_pack(static_cast<uint8_t>(meldedIdx + 1), PACK_TYPE_PUNG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 3; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 3;

    addFixedPungPack(tile, meldedIdx);
    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedMeldedKongPack() {
    return makeFixedDircetMeldedKongPack() || promoteFixedPungPack();
}

bool HandTilesWidget::makeFixedDircetMeldedKongPack() {
    if (UNLIKELY(!canDirectKong())) {
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    int meldedIdx = calcMeldedIdx(3);
    mahjong::pack_t pack = mahjong::make_pack(static_cast<uint8_t>(std::min(meldedIdx + 1, 3)), PACK_TYPE_KONG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 4; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 4;

    addFixedMeldedKongPack(tile, meldedIdx);

    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::promoteFixedPungPack() {
    if (UNLIKELY(!canPromotedKong())) {
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    auto it = std::find_if(_fixedPacks.begin(), _fixedPacks.end(), [tile](mahjong::pack_t pack) {
        return (mahjong::pack_get_type(pack) == PACK_TYPE_PUNG && mahjong::pack_get_tile(pack) == tile);
    });
    *it = mahjong::promote_pung_to_kong(*it);

    _standingTiles.erase(std::find(_standingTiles.begin(), _standingTiles.end(), tile));
    _standingTilesTable[tile] -= 1;

    promoteFixedPungPackToKongPack(tile, it - _fixedPacks.begin());

    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedConcealedKongPack() {
    if (UNLIKELY(!canDirectKong())) {
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(0, PACK_TYPE_KONG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 4; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 4;

    addFixedConcealedKongPack(tile);
    refreshStandingTiles();
    return true;
}

void HandTilesWidget::discardCurrentTile() {
    if (_currentIdx < _standingTiles.size()) {
        _standingTileButtons[_currentIdx]->setVisible(false);
    }
}

cocos2d::Node *HandTilesWidget::createStaticNode(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile) {
    const char *image[18];
    Vec2 pos[18];
    bool rotated[18] = { false };
    long tile_cnt = 0;
    int totalWidth = 0;

    // 副露
    for (long i = 0; i < handTiles.pack_count; ++i) {
        mahjong::pack_t pack = handTiles.fixed_packs[i];
        mahjong::tile_t tile = mahjong::pack_get_tile(pack);
        switch (mahjong::pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
            rotated[tile_cnt + 0] = true;

            pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
            pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
            pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

            switch (mahjong::pack_get_offer(pack)) {
            default:
                image[tile_cnt + 0] = tilesImageName[tile - 1];
                image[tile_cnt + 1] = tilesImageName[tile];
                image[tile_cnt + 2] = tilesImageName[tile + 1];
                break;
            case 2:
                image[tile_cnt + 0] = tilesImageName[tile];
                image[tile_cnt + 1] = tilesImageName[tile - 1];
                image[tile_cnt + 2] = tilesImageName[tile + 1];
                break;
            case 3:
                image[tile_cnt + 0] = tilesImageName[tile + 1];
                image[tile_cnt + 1] = tilesImageName[tile - 1];
                image[tile_cnt + 2] = tilesImageName[tile];
                break;
            }
            totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
            tile_cnt += 3;
            break;

        case PACK_TYPE_PUNG:
            image[tile_cnt + 0] = tilesImageName[tile];
            image[tile_cnt + 1] = tilesImageName[tile];
            image[tile_cnt + 2] = tilesImageName[tile];

            switch (mahjong::pack_get_offer(pack)) {
            default:
                rotated[tile_cnt + 0] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                break;
            case 2:
                rotated[tile_cnt + 1] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                break;
            case 3:
                rotated[tile_cnt + 2] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                break;
            }
            totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
            tile_cnt += 3;
            break;

        case PACK_TYPE_KONG:
            image[tile_cnt + 0] = tilesImageName[tile];
            image[tile_cnt + 1] = tilesImageName[tile];
            image[tile_cnt + 2] = tilesImageName[tile];
            image[tile_cnt + 3] = tilesImageName[tile];

            switch (mahjong::pack_get_offer(pack)) {
            case 0:
                image[tile_cnt + 0] = "tiles/bg.png";
                image[tile_cnt + 3] = "tiles/bg.png";

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 3] = Vec2(totalWidth + TILE_WIDTH * 3.5f, TILE_HEIGHT * 0.5f);

                totalWidth += (TILE_WIDTH * 4 + GAP);
                break;
            default:
                if (mahjong::is_promoted_kong(pack)) {
                    rotated[tile_cnt + 0] = true;
                    rotated[tile_cnt + 1] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 1.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

                    totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
                }
                else {
                    rotated[tile_cnt + 0] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);

                    totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                }
                break;
            case 2:
                if (mahjong::is_promoted_kong(pack)) {
                    rotated[tile_cnt + 1] = true;
                    rotated[tile_cnt + 2] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 1.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

                    totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
                }
                else {
                    rotated[tile_cnt + 1] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);

                    totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                }
                break;
            case 3:
                if (mahjong::is_promoted_kong(pack)) {
                    rotated[tile_cnt + 2] = true;
                    rotated[tile_cnt + 3] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 1.5f);

                    totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
                }
                else {
                    rotated[tile_cnt + 3] = true;

                    pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
                    pos[tile_cnt + 3] = Vec2(totalWidth + TILE_WIDTH * 3 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);

                    totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                }
                break;
            }
            tile_cnt += 4;
            break;

        default:
            break;
        }
    }

    // 立牌
    for (long i = 0; i < handTiles.tile_count; ++i) {
        mahjong::tile_t tile = handTiles.standing_tiles[i];
        image[tile_cnt] = tilesImageName[tile];
        pos[tile_cnt] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        totalWidth += TILE_WIDTH;
        ++tile_cnt;
    }

    // 上牌
    if (servingTile != 0 && totalWidth > 0) {
        totalWidth += GAP;

        image[tile_cnt] = tilesImageName[servingTile];
        pos[tile_cnt] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        totalWidth += TILE_WIDTH;
        ++tile_cnt;
    }

    Node *node = Node::create();
    node->setContentSize(Size(static_cast<float>(totalWidth), static_cast<float>(std::max(TILE_HEIGHT, TILE_WIDTH * 2))));
    node->setIgnoreAnchorPointForPosition(false);
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (long i = 0; i < tile_cnt; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        node->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (rotated[i]) {
            sprite->setRotation(-90);
        }
    }

    return node;
}

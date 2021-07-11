#include "OtherScene.h"
#include "../UICommon.h"
#include "../widget/CommonWebViewScene.h"
#include "../widget/LoadingView.h"
#include "../CompetitionSystem/LatestCompetitionScene.h"

USING_NS_CC;

static std::string g_text;

bool OtherScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("其他")))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("相关补充"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 50.0f));
    button->addClickEventListener(std::bind(&OtherScene::onTipsButton, this, std::placeholders::_1));

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("近期赛事"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
    button->addClickEventListener(std::bind(&OtherScene::onCompetitionButton, this, std::placeholders::_1));

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("娱乐消遣"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 50.0f));
    button->addClickEventListener(std::bind(&OtherScene::onRecreationsButton, this, std::placeholders::_1));

    return true;
}

void OtherScene::onTipsButton(cocos2d::Ref *) {
    if (LIKELY(!g_text.empty())) {
        Director::getInstance()->pushScene(
            CommonWebViewScene::create(__UTF8("相关补充"), g_text, CommonWebViewScene::ContentType::HTML));
        return;
    }

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    auto thiz = makeRef(this);  // 保证线程回来之前不析构

    auto text = std::make_shared<std::string>();
    AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, loadingView, text](void *) {
        g_text.swap(*text);
        if (LIKELY(thiz->isRunning())) {
            loadingView->dismiss();
            Director::getInstance()->pushScene(
                CommonWebViewScene::create(__UTF8("相关补充"), g_text, CommonWebViewScene::ContentType::HTML));
        }
    }, nullptr, [text]() {
        ValueMap valueMap = FileUtils::getInstance()->getValueMapFromFile("text/other.xml");
        if (LIKELY(!valueMap.empty())) {
            *text = valueMap.begin()->second.asString();
        }
    });
}

void OtherScene::onCompetitionButton(cocos2d::Ref *) {
    Director::getInstance()->pushScene(LatestCompetitionScene::create());
}

void OtherScene::onRecreationsButton(cocos2d::Ref *) {
    Director::getInstance()->pushScene(
        CommonWebViewScene::create(__UTF8("娱乐消遣"), "http://www.tziakcha.xyz/almanac.html", CommonWebViewScene::ContentType::URL));
}

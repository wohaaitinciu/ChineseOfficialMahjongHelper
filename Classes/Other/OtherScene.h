#ifndef __OTHER_SCENE_H__
#define __OTHER_SCENE_H__

#include "../BaseScene.h"

class OtherScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(OtherScene);

private:
    void onTipsButton(cocos2d::Ref *sender);
    void onCompetitionButton(cocos2d::Ref *sender);
    void onRecreationsButton(cocos2d::Ref *sender);
};

#endif

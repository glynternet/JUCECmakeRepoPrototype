#include "JuceHeader.h"

class RemoteAddressComponent : public Component {
   private:
    Label label;
    Label input;

    const Colour colour1 = Colours::white;
    const Colour colour2 = Colours::black;

   public:
    RemoteAddressComponent() {
        addAndMakeVisible(label);
        label.setText("Remote:", dontSendNotification);
        label.attachToComponent(&input, true);
        label.setColour(Label::textColourId, colour1);
        label.setColour(Label::backgroundColourId, colour2);
        label.setColour(Label::outlineColourId, colour1);
        label.setJustificationType(Justification::right);

        addAndMakeVisible(input);
        input.setEditable(false,  // editable on single-click
                          true,   // editable on double-click
                          true    // discard changes on loss of focus
        );
        input.setColour(Label::textColourId, colour1);
        input.setColour(Label::backgroundColourId, colour2);
        input.setColour(Label::outlineColourId, colour1);
        input.setColour(Label::textWhenEditingColourId, colour2);
        input.setColour(Label::backgroundWhenEditingColourId, colour1);
        input.setColour(Label::outlineWhenEditingColourId, Colours::black);
        input.onTextChange = [this] { onTextChange(input.getText()); };
        input.onEditorHide = [this]() { input.setJustificationType(Justification::right); };
        input.onEditorShow = [this]() { input.setJustificationType(Justification::left); };
    }
    ~RemoteAddressComponent() {}

    void resized() {
        const int labelWidth = 50;
        input.setBounds(getLocalBounds().removeFromRight(getWidth() - labelWidth));
    }

    void setText(const String &newText, NotificationType notification) {
        input.setText(newText, notification);
    }

    std::function<void(String)> onTextChange;
};
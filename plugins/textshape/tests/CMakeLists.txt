set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/plugins/textshape 
    ${CMAKE_SOURCE_DIR}/plugins/textshape/dialogs
    ${CMAKE_BINARY_DIR}/plugins/textshape/dialogs
    ${CMAKE_BINARY_DIR}/plugins/textshape
    ${KOTEXT_INCLUDES}
    ${PIGMENT_INCLUDES})


########### next target ###############

set(TestDocumentLayout_test_SRCS
    TestDocumentLayout.cpp
    TestLists.cpp
    ../TextShapeLayout.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../TextShape.cpp
    ../ListItemsHelper.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)

kde4_add_unit_test(TestDocumentLayout TESTNAME shapes-text-TestDocumentLayout ${TestDocumentLayout_test_SRCS})

target_link_libraries(TestDocumentLayout  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestChangeListCommand_test_SRCS
    TestChangeListCommand.cpp
    ../commands/TextCommandBase.cpp
    ../commands/TextCutCommand.cpp
    ../commands/ShowChangesCommand.cpp
    ../commands/AcceptChangeCommand.cpp
    ../commands/RejectChangeCommand.cpp
    ../commands/AutoResizeCommand.cpp
    ../TextTool.cpp
    ../TextEditingPluginContainer.cpp
    ../ChangeTracker.cpp
    ../dialogs/FontDia.cpp
    ../dialogs/InsertCharacter.cpp
    ../dialogs/StyleManagerDialog.cpp
    ../dialogs/SimpleStylesWidget.cpp
    ../dialogs/SimpleCharacterWidget.cpp
    ../dialogs/ParagraphSettingsDialog.cpp
    ../dialogs/SimpleParagraphWidget.cpp
    ../dialogs/FormattingButton.cpp
    ../dialogs/SimpleTableWidget.cpp
    ../dialogs/QuickTableButton.cpp
    ../dialogs/StylesWidget.cpp
    ../dialogs/FontTab.cpp
    ../dialogs/CharacterHighlighting.cpp
    ../dialogs/FontDecorations.cpp
    ../dialogs/FontLayoutTab.cpp
    ../dialogs/LanguageTab.cpp
    ../dialogs/StyleManager.cpp
    ../dialogs/ParagraphIndentSpacing.cpp
    ../dialogs/ParagraphLayout.cpp
    ../dialogs/ParagraphDecorations.cpp
    ../dialogs/ParagraphBulletsNumbers.cpp
    ../dialogs/ListsSpinBox.cpp
    ../dialogs/StylesModel.cpp
    ../dialogs/StylesDelegate.cpp
    ../dialogs/ParagraphGeneral.cpp
    ../dialogs/CharacterGeneral.cpp
    ../dialogs/NewStyleWidget.cpp
    ../dialogs/FormattingPreview.cpp
    ../dialogs/StyleManagerWelcome.cpp
    ../dialogs/TableDialog.cpp
    ../dialogs/TrackedChangeManager.cpp
    ../dialogs/TrackedChangeModel.cpp
    ../dialogs/ChangeConfigureDialog.cpp
    ../dialogs/AcceptRejectChangeDialog.cpp
    ../dialogs/ChangeTrackingOptionsWidget.cpp
    ../dialogs/SimpleTableWidget.cpp
    ../ListItemsHelper.cpp
    ../TextShape.cpp
    ../TextShapeLayout.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../FontSizeAction.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)


kde4_add_unit_test(TestChangeListCommand TESTNAME shapes-text-TestChangeListCommand ${TestChangeListCommand_test_SRCS})
target_link_libraries(TestChangeListCommand  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestChangesDatabase_test_SRCS
    TestChangesDatabase.cpp
    ../TextChanges.cpp
    ../TextChange.cpp
)
kde4_add_unit_test(TestChangesDatabase TESTNAME shapes-text-TestChangesDatabase ${TestChangesDatabase_test_SRCS})
target_link_libraries(TestChangesDatabase ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})

########### next target ###############

#set(TestStylesModel_test_SRCS
#    TestStylesModel.cpp
#    ../dialogs/StylesModel.cpp
#    ../TextShape.cpp
#    ../Layout.cpp
#    ../TextLine.cpp
#    ../Outline.cpp
#    ../TableLayout.cpp
#    ../TableLayoutData.cpp
#    ../ListItemsHelper.cpp
#    ../ToCGenerator.cpp
#    ../ShrinkToFitShapeContainer.cpp
#    ../FloatingAnchorStrategy.cpp
#    ../InlineAnchorStrategy.cpp
#)
#kde4_add_unit_test(TestStylesModel TESTNAME shapes-text-TestStylesModel ${TestStylesModel_test_SRCS})
#target_link_libraries(TestStylesModel kotext  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestTableLayout_test_SRCS
    TestTableLayout.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../TextShapeLayout.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../TextShape.cpp
    ../ListItemsHelper.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)
kde4_add_unit_test(TestTableLayout TESTNAME shapes-text-TestTableLayout ${TestTableLayout_test_SRCS})
target_link_libraries(TestTableLayout kotext  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestSections_test_SRCS
    TestSections.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../TextShapeLayout.cpp
    ../TextShape.cpp
    ../ListItemsHelper.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)
kde4_add_unit_test(TestSections TESTNAME shapes-text-TestSections ${TestSections_test_SRCS})
target_link_libraries(TestSections kotext  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestTextTool_test_SRCS
    TestTextTool.cpp
    ../ChangeTracker.cpp
    ../commands/AcceptChangeCommand.cpp
    ../commands/ChangeListCommand.cpp
    ../commands/ChangeListLevelCommand.cpp
    ../commands/ChangeTrackedDeleteCommand.cpp
    ../commands/DeleteCommand.cpp
    ../commands/ListItemNumberingCommand.cpp
    ../commands/RejectChangeCommand.cpp
    ../commands/ShowChangesCommand.cpp
    ../commands/TextCommandBase.cpp
    ../commands/TextCutCommand.cpp
    ../commands/TextPasteCommand.cpp
    ../commands/AutoResizeCommand.cpp
    ../dialogs/ChangeConfigureDialog.cpp
    ../dialogs/AcceptRejectChangeDialog.cpp
    ../dialogs/CharacterGeneral.cpp
    ../dialogs/CharacterHighlighting.cpp
    ../dialogs/FontDecorations.cpp
    ../dialogs/FontDia.cpp
    ../dialogs/FontLayoutTab.cpp
    ../dialogs/FontTab.cpp
    ../dialogs/FormattingPreview.cpp
    ../dialogs/InsertCharacter.cpp
    ../dialogs/LanguageTab.cpp
    ../dialogs/ListsSpinBox.cpp
    ../dialogs/NewStyleWidget.cpp
    ../dialogs/ParagraphBulletsNumbers.cpp
    ../dialogs/ParagraphDecorations.cpp
    ../dialogs/ParagraphGeneral.cpp
    ../dialogs/ParagraphIndentSpacing.cpp
    ../dialogs/ParagraphLayout.cpp
    ../dialogs/ParagraphSettingsDialog.cpp
    ../dialogs/StyleManager.cpp
    ../dialogs/StyleManagerDialog.cpp
    ../dialogs/StyleManagerWelcome.cpp
    ../dialogs/StylesModel.cpp
    ../dialogs/StylesDelegate.cpp
    ../dialogs/SimpleStylesWidget.cpp
    ../dialogs/SimpleCharacterWidget.cpp
    ../dialogs/SimpleParagraphWidget.cpp
    ../dialogs/FormattingButton.cpp
    ../dialogs/SimpleTableWidget.cpp
    ../dialogs/QuickTableButton.cpp
    ../dialogs/StylesWidget.cpp
    ../dialogs/TableDialog.cpp
    ../dialogs/TrackedChangeManager.cpp
    ../dialogs/TrackedChangeModel.cpp
    ../dialogs/ChangeTrackingOptionsWidget.cpp
    ../TextShapeLayout.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../ListItemsHelper.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../TextEditingPluginContainer.cpp
    ../TextShape.cpp
    ../TextTool.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../FontSizeAction.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)
kde4_add_unit_test(TestTextTool TESTNAME shapes-text-TestTextTool ${TestTextTool_test_SRCS})
target_link_libraries(TestTextTool  ${QT_QTTEST_LIBRARY})

########### next target ###############

set(TestChangeTrackedDelete_test_SRCS
    TestChangeTrackedDelete.cpp
    ../commands/ChangeListCommand.cpp
    ../commands/ChangeListLevelCommand.cpp
    ../commands/ListItemNumberingCommand.cpp
    ../commands/TextCommandBase.cpp
    ../commands/TextPasteCommand.cpp
    ../commands/TextCutCommand.cpp
    ../commands/ChangeTrackedDeleteCommand.cpp
    ../commands/DeleteCommand.cpp
    ../commands/ShowChangesCommand.cpp
    ../commands/AcceptChangeCommand.cpp
    ../commands/RejectChangeCommand.cpp
    ../commands/AutoResizeCommand.cpp
    ../TextTool.cpp
    ../TextEditingPluginContainer.cpp
    ../ChangeTracker.cpp
    ../dialogs/FontDia.cpp
    ../dialogs/InsertCharacter.cpp
    ../dialogs/StyleManagerDialog.cpp
    ../dialogs/ParagraphSettingsDialog.cpp
    ../dialogs/SimpleStylesWidget.cpp
    ../dialogs/SimpleCharacterWidget.cpp
    ../dialogs/SimpleParagraphWidget.cpp
    ../dialogs/FormattingButton.cpp
    ../dialogs/SimpleTableWidget.cpp
    ../dialogs/QuickTableButton.cpp
    ../dialogs/StylesWidget.cpp
    ../dialogs/FontTab.cpp
    ../dialogs/CharacterHighlighting.cpp
    ../dialogs/FontDecorations.cpp
    ../dialogs/FontLayoutTab.cpp
    ../dialogs/LanguageTab.cpp
    ../dialogs/StyleManager.cpp
    ../dialogs/ParagraphIndentSpacing.cpp
    ../dialogs/ParagraphLayout.cpp
    ../dialogs/ParagraphDecorations.cpp
    ../dialogs/ParagraphBulletsNumbers.cpp
    ../dialogs/ListsSpinBox.cpp
    ../dialogs/StylesModel.cpp
    ../dialogs/StylesDelegate.cpp
    ../dialogs/ParagraphGeneral.cpp
    ../dialogs/CharacterGeneral.cpp
    ../dialogs/NewStyleWidget.cpp
    ../dialogs/FormattingPreview.cpp
    ../dialogs/StyleManagerWelcome.cpp
    ../dialogs/TableDialog.cpp
    ../dialogs/TrackedChangeManager.cpp
    ../dialogs/TrackedChangeModel.cpp
    ../dialogs/ChangeConfigureDialog.cpp
    ../dialogs/AcceptRejectChangeDialog.cpp
    ../dialogs/ChangeTrackingOptionsWidget.cpp
    ../dialogs/SimpleTableWidget.cpp
    ../ListItemsHelper.cpp
    ../TextShape.cpp
    ../TextShapeLayout.cpp
    ../TextLine.cpp
    ../Outline.cpp
    ../TableLayout.cpp
    ../TableLayoutData.cpp
    ../ToCGenerator.cpp
    ../ShrinkToFitShapeContainer.cpp
    ../FontSizeAction.cpp
    ../FloatingAnchorStrategy.cpp
    ../InlineAnchorStrategy.cpp
)
kde4_add_unit_test(TestChangeTrackedDelete TESTNAME shapes-text-TestChangeTrackedDelete ${TestChangeTrackedDelete_test_SRCS})
target_link_libraries(TestChangeTrackedDelete  kotext ${QT_QTTEST_LIBRARY})


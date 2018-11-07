open AssetNodeType;

open AssetTreeNodeType;

module Method = {
  let _isSelected = nodeId =>
    AssetTreeUtils.getTargetTreeNodeId
    |> StateLogicService.getEditorState === nodeId;

  let _isActive = () => {
    let editorState = StateEditorService.getState();

    switch (CurrentNodeDataAssetEditorService.getCurrentNodeData(editorState)) {
    | None => false
    | Some({currentNodeId}) =>
      TreeAssetEditorService.isIdEqual(
        AssetTreeUtils.getTargetTreeNodeId(editorState),
        currentNodeId,
      )
    };
  };

  let _isNotRoot = nodeId =>
    StateEditorService.getState()
    |> TreeRootAssetEditorService.getRootTreeNodeId != nodeId;

  let handleToggleShowTreeChildren =
      (store, dispatchFunc, targetId, isShowChildren) => {
    let editorState = StateEditorService.getState();

    AssetTreeUtils.setSpecificAssetTreeNodeIsShowChildren(
      targetId,
      isShowChildren,
      [|editorState |> TreeRootAssetEditorService.unsafeGetAssetTreeRoot|],
    )
    |> ArrayService.unsafeGetFirst
    |. TreeRootAssetEditorService.setAssetTreeRoot(editorState)
    |> StateEditorService.setState;

    dispatchFunc(
      AppStore.UpdateAction(Update([|UpdateStore.BottomComponent|])),
    )
    |> ignore;
  };

  let buildAssetTreeArray =
      (
        (store, dispatchFunc, dragImg),
        (onSelectFunc, onDropFunc),
        assetTreeRoot,
      ) => {
    let rec _iterateAssetTreeArray = assetTreeArray =>
      assetTreeArray
      |> Js.Array.map(({nodeId, type_, isShowChildren, children}) =>
           switch (type_) {
           | Folder =>
             let {name}: folderResultType =
               StateEditorService.getState()
               |> FolderNodeMapAssetEditorService.getFolderNodeMap
               |> WonderCommonlib.SparseMapService.unsafeGet(nodeId);

             <AssetTreeNode
               key=(DomHelper.getRandomKey())
               id=nodeId
               name
               isSelected=(_isSelected(nodeId))
               isActive=(_isActive())
               dragImg
               widget=(AssetUtils.getWidget())
               icon="./public/img/package.png"
               isDragable=(_isNotRoot(nodeId))
               onSelect=(onSelectFunc(type_))
               onDrop=onDropFunc
               isWidget=AssetUtils.isWidget
               isShowChildren
               isHasChildren=(
                 children
                 |> Js.Array.filter(({type_}) => type_ === Folder)
                 |> Js.Array.length >= 1
               )
               handleToggleShowTreeChildren=(
                 handleToggleShowTreeChildren(store, dispatchFunc)
               )
               handleRelationError=AssetTreeUtils.isTreeNodeRelationError
               treeChildren=(_iterateAssetTreeArray(children))
             />;

           | _ => ReasonReact.null
           }
         );
    _iterateAssetTreeArray([|assetTreeRoot|]);
  };
};

let component = ReasonReact.statelessComponent("AssetTree");

let render = ((store, dispatchFunc), dragImg, _self) =>
  <article key="assetTreeRoot" className="wonder-asset-assetTree">
    (
      ReasonReact.array(
        StateEditorService.getState()
        |> TreeRootAssetEditorService.unsafeGetAssetTreeRoot
        |> Method.buildAssetTreeArray(
             (store, dispatchFunc, dragImg),
             (
               AssetTreeUtils.enterFolder(dispatchFunc),
               AssetDragNodeToFolderEventHandler.MakeEventHandler.pushUndoStackWithNoCopyEngineState(
                 (store, dispatchFunc),
                 (),
               ),
             ),
           ),
      )
    )
  </article>;

let make = (~store, ~dispatchFunc, ~dragImg, _children) => {
  ...component,
  render: self => render((store, dispatchFunc), dragImg, self),
};
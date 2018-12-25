open EditorType;

let getTree = ({assetRecord}) => assetRecord.tree;

let unsafeGetTree = editorState =>
  editorState |> getTree |> OptionService.unsafeGet;

let setTree = (tree, {assetRecord} as editorState) => {
  ...editorState,
  assetRecord: {
    ...assetRecord,
    tree: Some(tree),
  },
};

let clearTree = ({assetRecord} as editorState) => {
  ...editorState,
  assetRecord: {
    ...assetRecord,
    tree: None,
  },
};

let createTree = editorState => {
  let (_, newTree, newIndex) =
    RootTreeAssetService.buildRootNode(
      RootTreeAssetService.getAssetTreeRootName(),
      IndexAssetEditorService.getNodeIndex(editorState),
    );

  editorState
  |> setTree(newTree)
  |> IndexAssetEditorService.setNodeIndex(newIndex);
};

let getSelectedFolderNodeInAssetTree = editorState =>
  switch (
    SelectedFolderNodeInAssetTreeAssetEditorService.getSelectedFolderNodeInAssetTree(
      editorState,
    )
  ) {
  | None => RootTreeAssetService.getRootNode(unsafeGetTree(editorState))
  | Some(node) => node
  };
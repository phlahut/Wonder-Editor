let getChildren = (gameObject) =>
  StateLogicService.getRunEngineState() |> GameObjectUtils.getChildren(gameObject);

let getEditEngineChildren = (gameObject) =>
  StateLogicService.getEditEngineState() |> GameObjectUtils.getChildren(gameObject);

let unsafeGetCurrentSceneTreeNode = () =>
  SceneEditorService.unsafeGetCurrentSceneTreeNode |> StateLogicService.getEditorState;

let clearCurrentSceneTreeNode = () =>
  SceneEditorService.clearCurrentSceneTreeNode |> StateLogicService.getAndSetEditorState;

let addFakeVboBufferForGameObject = (gameObject) => {
  StateLogicService.getEditEngineState()
  |> MainEditorVboBufferTool.passBufferShouldExistCheckWhenDisposeGeometry(
       GameObjectComponentEngineService.getCustomGeometryComponent(gameObject)
       |> StateLogicService.getEngineStateToGetData
     )
  |> StateLogicService.setEditEngineState;
  StateLogicService.getRunEngineState()
  |> MainEditorVboBufferTool.passBufferShouldExistCheckWhenDisposeGeometry(
       GameObjectComponentEngineService.getCustomGeometryComponent(gameObject)
       |> StateLogicService.getEngineStateToGetData
     )
  |> StateLogicService.setRunEngineState
  |> ignore
};

let getCurrentSceneTreeNodeTransform = () =>
  GameObjectComponentEngineService.getTransformComponent(unsafeGetCurrentSceneTreeNode())
  |> StateLogicService.getEngineStateToGetData;

let getCurrentSceneTreeNodeMaterial = () =>
  GameObjectComponentEngineService.getBasicMaterialComponent(unsafeGetCurrentSceneTreeNode())
  |> StateLogicService.getEngineStateToGetData;

let getCurrentSceneTreeNode = () =>
  SceneEditorService.getCurrentSceneTreeNode |> StateLogicService.getEditorState;

let setCurrentSceneTreeNode = (gameObject) =>
  SceneEditorService.setCurrentSceneTreeNode(gameObject) |> StateLogicService.getAndSetEditorState;
/* let hasCurrentSceneTreeNode = () =>
   SceneEditorService.hasCurrentSceneTreeNode |> StateLogicService.getEditorState; */
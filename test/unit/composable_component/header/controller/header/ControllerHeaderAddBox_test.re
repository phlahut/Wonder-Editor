open Wonder_jest;

open Expect;

open Expect.Operators;

open Sinon;

let _ =
  describe("controller header addBox", () => {
    let sandbox = getSandboxDefaultVal();
    beforeEach(() => {
      sandbox := createSandbox();
      MainEditorSceneTool.initState(~sandbox, ());
      MainEditorSceneTool.createDefaultScene(
        sandbox,
        MainEditorSceneTool.setFirstBoxTobeCurrentSceneTreeNode,
      );

      ControllerTool.stubRequestAnimationFrame(
        createEmptyStubWithJsObjSandbox(sandbox),
      );
      ControllerTool.run();
    });
    afterEach(() => {
      restoreSandbox(refJsObjToSandbox(sandbox^));
    });
    describe("test add box", () =>
      describe(
        "box should be added into EditEngineState and RunEngineState", () => {
        test("test add one box", () => {
          HeaderTool.triggerAddBox();


          (
            StateLogicService.getEditEngineState()
            |> GameObjectUtils.getChildren(
                 MainEditorSceneTool.unsafeGetScene(),
               )
            |> Js.Array.length,
            StateLogicService.getRunEngineState()
            |> GameObjectUtils.getChildren(
                 MainEditorSceneTool.unsafeGetScene(),
               )
            |> Js.Array.length,
          )
          |> expect == (6, 5);
        });
        test("test add two boxes", () => {
          HeaderTool.triggerAddBox();
          HeaderTool.triggerAddBox();


          (
            StateLogicService.getEditEngineState()
            |> GameObjectUtils.getChildren(
                 MainEditorSceneTool.unsafeGetScene(),
               )
            |> Js.Array.length,
            StateLogicService.getRunEngineState()
            |> GameObjectUtils.getChildren(
                 MainEditorSceneTool.unsafeGetScene(),
               )
            |> Js.Array.length,
          )
          |> expect == (7, 6);
        });
        describe("test scene tree", () =>
          test("test add one box", () => {
          HeaderTool.triggerAddBox();


            BuildComponentTool.buildSceneTree(
              TestTool.buildAppStateSceneGraphFromEngine(),
            )
            |> ReactTestTool.createSnapshotAndMatch;
          })
        );
      })
    );
  });
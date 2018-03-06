'use strict';

import * as MainEditorCameraOper$WonderEditor      from "./MainEditorCameraOper.js";
import * as PrimitiveCompositeService$WonderEditor from "../../../../../service/logic_service/composite/PrimitiveCompositeService.js";

function createDefaultSceneGameObjects(state) {
  var match = PrimitiveCompositeService$WonderEditor.createBox(state);
  var match$1 = PrimitiveCompositeService$WonderEditor.createBox(match[0]);
  var match$2 = MainEditorCameraOper$WonderEditor.createCamera(match$1[0]);
  return /* tuple */[
          match$2[0],
          match$2[1],
          match[1],
          match$1[1]
        ];
}

export {
  createDefaultSceneGameObjects ,
  
}
/* MainEditorCameraOper-WonderEditor Not a pure module */
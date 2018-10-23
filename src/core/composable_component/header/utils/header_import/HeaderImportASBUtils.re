open Js.Typed_array;

open Js.Promise;

let _getJsonStr = (jsonByteLength, asb) => {
  let decoder = TextDecoder.newTextDecoder("utf-8");

  decoder
  |> TextDecoder.decodeUint8Array(
       Uint8Array.fromBufferRange(
         asb,
         ~offset=ASBUtils.getHeaderTotalByteLength(),
         ~length=jsonByteLength,
       ),
     );
};

let _getBuffer = (jsonByteLength, asb) =>
  asb
  |> ArrayBuffer.sliceFrom(
       ASBUtils.getHeaderTotalByteLength()
       + jsonByteLength
       |> BufferUtils.alignedLength,
     );

let _buildLoadImageStream = (arrayBuffer, mimeType, errorMsg) => {
  let blob = Blob.newBlobFromArrayBuffer(arrayBuffer, mimeType);

  LoadImageUtils.loadBlobImage(blob |> Blob.createObjectURL, errorMsg)
  |> WonderBsMost.Most.tap(image => Blob.revokeObjectURL(blob));
};

let _getArrayBuffer =
    (buffer, bufferView, bufferViews: array(ExportAssetType.bufferView)) => {
  let {byteOffset, byteLength}: ExportAssetType.bufferView =
    Array.unsafe_get(bufferViews, bufferView);

  buffer
  |> Js.Typed_array.ArrayBuffer.slice(
       ~start=byteOffset,
       ~end_=byteOffset + byteLength,
     );
};

let _buildImageData =
    ({images, bufferViews}: ExportAssetType.assets, buffer, editorState) =>
  images
  |> WonderCommonlib.ArrayService.reduceOneParami(
       (.
         streamArr,
         {name, bufferView, mimeType}: ExportAssetType.image,
         imageIndex,
       ) => {
         let arrayBuffer = _getArrayBuffer(buffer, bufferView, bufferViews);

         streamArr
         |> ArrayService.push(
              _buildLoadImageStream(
                arrayBuffer,
                mimeType,
                {j|load image error. imageIndex: $imageIndex|j},
              )
              |> WonderBsMost.Most.map(image => {
                   /* TODO test */
                   ImageUtils.setImageName(image, name);

                   image;
                 })
              |> WonderBsMost.Most.map(image =>
                   (
                     image,
                     Uint8Array.fromBuffer(arrayBuffer),
                     imageIndex,
                     name,
                     mimeType,
                   )
                 ),
            );
       },
       [||],
     )
  /* |> WonderBsMost.Most.from */
  |> WonderBsMost.Most.mergeArray
  |> WonderBsMost.Most.reduce(
       (
         (imageMap, imageNodeIdMap, editorState),
         (image, uint8Array, imageIndex, name, mimeType),
       ) => {
         let (editorState, assetNodeId) =
           AssetIdUtils.generateAssetId(editorState);

         (
           imageMap |> WonderCommonlib.SparseMapService.set(imageIndex, image),
           imageNodeIdMap
           |> WonderCommonlib.SparseMapService.set(imageIndex, assetNodeId),
           editorState
           |> AssetImageNodeMapEditorService.setResult(
                assetNodeId,
                AssetImageNodeMapEditorService.buildImageNodeResult(
                  None,
                  Some(uint8Array),
                  name,
                  mimeType,
                ),
              ),
         );
       },
       (
         WonderCommonlib.SparseMapService.createEmpty(),
         WonderCommonlib.SparseMapService.createEmpty(),
         editorState,
       ),
     );

let _buildTextureData =
    (
      {textures}: ExportAssetType.assets,
      (imageMap, imageNodeIdMap),
      (editorState, engineState),
    ) =>
  textures
  |> WonderCommonlib.ArrayService.reduceOneParami(
       (.
         (textureMap, (editorState, engineState)),
         {path, source, name, magFilter, minFilter, wrapS, wrapT}: ExportAssetType.texture,
         textureIndex,
       ) => {
         let (engineState, texture) =
           BasicSourceTextureEngineService.create(engineState);

         let engineState =
           engineState
           |> BasicSourceTextureEngineService.setWrapS(
                wrapS |> TextureTypeUtils.convertIntToWrap,
                texture,
              )
           |> BasicSourceTextureEngineService.setWrapT(
                wrapT |> TextureTypeUtils.convertIntToWrap,
                texture,
              )
           |> BasicSourceTextureEngineService.setMagFilter(
                magFilter |> TextureTypeUtils.convertIntToFilter,
                texture,
              )
           |> BasicSourceTextureEngineService.setMinFilter(
                minFilter |> TextureTypeUtils.convertIntToFilter,
                texture,
              )
           |> BasicSourceTextureEngineService.setBasicSourceTextureName(
                name,
                texture,
              )
           |> BasicSourceTextureEngineService.setSource(
                imageMap
                |> WonderCommonlib.SparseMapService.unsafeGet(source)
                |> ImageType.convertDomToImageElement,
                texture,
              );

         let (editorState, assetNodeId) =
           AssetIdUtils.generateAssetId(editorState);

         let (parentFolderNodeId, editorState) =
           HeaderImportFolderUtils.buildFolder(
             path,
             (editorState, engineState),
           );

         let editorState =
           editorState
           |> AssetTextureNodeMapEditorService.setResult(
                assetNodeId,
                AssetTextureNodeMapEditorService.buildTextureNodeResult(
                  texture,
                  parentFolderNodeId,
                  imageNodeIdMap
                  |> WonderCommonlib.SparseMapService.unsafeGet(source),
                ),
              )
           |> AssetTreeNodeUtils.createNodeAndAddToTargetNodeChildren(
                parentFolderNodeId |> OptionService.unsafeGet,
                assetNodeId,
                AssetNodeType.Texture,
              );

         (
           textureMap
           |> WonderCommonlib.SparseMapService.set(textureIndex, texture),
           (editorState, engineState),
         );
       },
       (
         WonderCommonlib.SparseMapService.createEmpty(),
         (editorState, engineState),
       ),
     );

let _buildMaterialEditorData =
    (material, path, type_, (editorState, engineState)) => {
  let (editorState, assetNodeId) = AssetIdUtils.generateAssetId(editorState);

  let (parentFolderNodeId, editorState) =
    HeaderImportFolderUtils.buildFolder(path, (editorState, engineState));

  editorState
  |> AssetMaterialNodeMapEditorService.setResult(
       assetNodeId,
       AssetMaterialNodeMapEditorService.buildMaterialNodeResult(
         parentFolderNodeId,
         type_,
         material,
       ),
     )
  |> AssetTreeNodeUtils.createNodeAndAddToTargetNodeChildren(
       parentFolderNodeId |> OptionService.unsafeGet,
       assetNodeId,
       AssetNodeType.Material,
     );
};

let _buildMaterialData =
    (
      {basicMaterials, lightMaterials}: ExportAssetType.assets,
      textureMap,
      (editorState, engineState),
    ) => {
  let (basicMaterialMap, (editorState, engineState)) =
    basicMaterials
    |> WonderCommonlib.ArrayService.reduceOneParami(
         (.
           (basicMaterialMap, (editorState, engineState)),
           {name, path, color, map}: ExportAssetType.basicMaterial,
           materialIndex,
         ) => {
           let (engineState, material) =
             BasicMaterialEngineService.create(engineState);

           let engineState =
             engineState
             |> BasicMaterialEngineService.setBasicMaterialName(
                  material,
                  name,
                )
             |> BasicMaterialEngineService.setColor(color, material);

           let engineState =
             switch (map) {
             | None => engineState
             | Some(map) =>
               engineState
               |> BasicMaterialEngineService.setBasicMaterialMap(
                    textureMap
                    |> WonderCommonlib.SparseMapService.unsafeGet(map),
                    material,
                  )
             };

           let editorState =
             _buildMaterialEditorData(
               material,
               path,
               AssetMaterialDataType.BasicMaterial,
               (editorState, engineState),
             );

           (
             basicMaterialMap
             |> WonderCommonlib.SparseMapService.set(materialIndex, material),
             (editorState, engineState),
           );
         },
         (
           WonderCommonlib.SparseMapService.createEmpty(),
           (editorState, engineState),
         ),
       );

  let (lightMaterialMap, (editorState, engineState)) =
    lightMaterials
    |> WonderCommonlib.ArrayService.reduceOneParami(
         (.
           (lightMaterialMap, (editorState, engineState)),
           {name, diffuseColor, diffuseMap, shininess, path}: ExportAssetType.lightMaterial,
           materialIndex,
         ) => {
           let (engineState, material) =
             LightMaterialEngineService.create(engineState);

           let engineState =
             engineState
             |> LightMaterialEngineService.setLightMaterialName(
                  material,
                  name,
                )
             |> LightMaterialEngineService.setLightMaterialDiffuseColor(
                  diffuseColor,
                  material,
                )
             |> LightMaterialEngineService.setLightMaterialShininess(
                  shininess,
                  material,
                );

           let engineState =
             switch (diffuseMap) {
             | None => engineState
             | Some(diffuseMap) =>
               engineState
               |> LightMaterialEngineService.setLightMaterialDiffuseMap(
                    textureMap
                    |> WonderCommonlib.SparseMapService.unsafeGet(diffuseMap),
                    material,
                  )
             };

           let editorState =
             _buildMaterialEditorData(
               material,
               path,
               AssetMaterialDataType.LightMaterial,
               (editorState, engineState),
             );

           (
             lightMaterialMap
             |> WonderCommonlib.SparseMapService.set(materialIndex, material),
             (editorState, engineState),
           );
         },
         (
           WonderCommonlib.SparseMapService.createEmpty(),
           (editorState, engineState),
         ),
       );

  (basicMaterialMap, lightMaterialMap, (editorState, engineState));
};

/* let _buildWDBData =
     (
       {wdbs, bufferViews}: ExportAssetType.assets,
       buffer,
       (editorState, engineState),
     ) =>
   wdbs
   /* |> WonderBsMost.Most.from
      |> WonderBsMost.Most.reduce( */
   |> WonderCommonlib.ArrayService.reduceOneParam(
        /* (allGameObjectsArr, (editorState, engineState)), */
        (. streamArr, {name, bufferView, path}: ExportAssetType.wdb) => {
          let arrayBuffer = _getArrayBuffer(buffer, bufferView, bufferViews);

          let (editorState, assetNodeId) =
            AssetIdUtils.generateAssetId(editorState);

          let (parentFolderNodeId, editorState) =
            HeaderImportFolderUtils.buildFolder(
              path,
              (editorState, engineState),
            );

          streamArr
          |> ArrayService.push(
               AssetWDBUtils.importAssetWDB(
                 (name, arrayBuffer),
                 (assetNodeId, parentFolderNodeId |> OptionService.unsafeGet),
                 (editorState, engineState),
               ),
             );
          /* |> then_(((allGameObjects, editorState, engineState)) =>
               (
                 allGameObjectsArr |> ArrayService.push(allGameObjects),
                 (editorState, engineState),
               )
               |> resolve
             ); */
        },
        /* ([||], (editorState, engineState)), */
        [||],
      )
   |> WonderBsMost.Most.mergeArray
   |> WonderBsMost.Most.reduce(
        (
          (allGameObjectsArr, (editorState, engineState)),
          (allGameObjects, (editorState, engineState)),
        ) => {



        },
        ([||], (editorState, engineState)),
      ); */

let _buildWDBData =
    (
      {wdbs, bufferViews}: ExportAssetType.assets,
      buffer,
      (editorState, engineState),
    ) => {
  editorState |> StateEditorService.setState |> ignore;
  engineState |> StateEngineService.setState |> ignore;
  let allGameObjectsArr = [||];

  wdbs
  |> WonderBsMost.Most.from
  |> WonderBsMost.Most.concatMap(
       ({name, bufferView, path}: ExportAssetType.wdb) => {
       let editorState = StateEditorService.getState();
       let engineState = StateEngineService.unsafeGetState();

       let arrayBuffer = _getArrayBuffer(buffer, bufferView, bufferViews);

       let (editorState, assetNodeId) =
         AssetIdUtils.generateAssetId(editorState);

       let (parentFolderNodeId, editorState) =
         HeaderImportFolderUtils.buildFolder(
           path,
           (editorState, engineState),
         );

       AssetWDBUtils.importAssetWDB(
         (name, arrayBuffer),
         (assetNodeId, parentFolderNodeId |> OptionService.unsafeGet),
         (editorState, engineState),
       )
       |> then_(((allGameObjects, (editorState, engineState))) => {
            editorState |> StateEditorService.setState |> ignore;
            engineState |> StateEngineService.setState |> ignore;

            allGameObjectsArr |> ArrayService.push(allGameObjects) |> ignore;

            () |> resolve;
          })
       |> WonderBsMost.Most.fromPromise;
     })
  |> WonderBsMost.Most.drain
  |> then_(_ => {
       let editorState = StateEditorService.getState();
       let engineState = StateEngineService.unsafeGetState();

       (allGameObjectsArr, (editorState, engineState)) |> resolve;
     });
  /* |> WonderCommonlib.ArrayService.reduceOneParam(
       (.
         streamArr,
         {name, bufferView, path}: ExportAssetType.wdb,
       ) => {
         let arrayBuffer = _getArrayBuffer(buffer, bufferView, bufferViews);

         let (editorState, assetNodeId) =
           AssetIdUtils.generateAssetId(editorState);

         let (parentFolderNodeId, editorState) =
           HeaderImportFolderUtils.buildFolder(
             path,
             (editorState, engineState),
           );


         streamArr
         |> ArrayService.push(
         AssetWDBUtils.importAssetWDB(
           (name, arrayBuffer),
           (assetNodeId, parentFolderNodeId |> OptionService.unsafeGet),
           /* (editorState, engineState), */
         )
            );
       },
       [||],
     ) */
  /* |> WonderBsMost.Most.from */
  /* |> WonderBsMost.Most.concatArray */
  /* |> WonderBsMost.Most.reduce(
       (
         (imageMap, imageNodeIdMap, editorState),
         (image, uint8Array, imageIndex, name, mimeType),
       ) => {
       },
       (
         WonderCommonlib.SparseMapService.createEmpty(),
         WonderCommonlib.SparseMapService.createEmpty(),
         editorState,
       ),
     ); */
  /* wdbs
     |> WonderBsMost.Most.from
     |> WonderBsMost.Most.reduce(
          (
            (allGameObjectsArr, (editorState, engineState)),
            {name, bufferView, path}: ExportAssetType.wdb,
          ) => {
            let arrayBuffer = _getArrayBuffer(buffer, bufferView, bufferViews);

            let (editorState, assetNodeId) =
              AssetIdUtils.generateAssetId(editorState);

            let (parentFolderNodeId, editorState) =
              HeaderImportFolderUtils.buildFolder(
                path,
                (editorState, engineState),
              );

            AssetWDBUtils.importAssetWDB(
              (name, arrayBuffer),
              (assetNodeId, parentFolderNodeId |> OptionService.unsafeGet),
              (editorState, engineState),
            )
            |> then_(((allGameObjects, (editorState, engineState))) =>
                 (
                   allGameObjectsArr |> ArrayService.push(allGameObjects),
                   (editorState, engineState),
                 )
                 |> resolve
               );
          },
          ([||], (editorState, engineState)) |> resolve,
        );
       }; */
};

let importASB = asb => {
  let editorState = StateEditorService.getState();
  let engineState = StateEngineService.unsafeGetState();

  let dataView = DataViewUtils.create(asb);

  let (jsonByteLength, byteOffset) =
    DataViewUtils.getUint32_1(. 0, dataView);

  let (bufferByteLength, byteOffset) =
    DataViewUtils.getUint32_1(. byteOffset, dataView);

  let jsonStr = _getJsonStr(jsonByteLength, asb);
  let buffer = _getBuffer(jsonByteLength, asb);

  let asbRecord: ExportAssetType.assets =
    jsonStr |> Js.Json.parseExn |> Obj.magic;

  _buildImageData(asbRecord, buffer, editorState)
  |> WonderBsMost.Most.fromPromise
  |> WonderBsMost.Most.map(((imageMap, imageNodeIdMap, editorState)) => {
       let (textureMap, (editorState, engineState)) =
         _buildTextureData(
           asbRecord,
           (imageMap, imageNodeIdMap),
           (editorState, engineState),
         );

       _buildMaterialData(asbRecord, textureMap, (editorState, engineState));
     })
  |> WonderBsMost.Most.flatMap(
       ((basicMaterialMap, lightMaterialMap, (editorState, engineState))) =>
       _buildWDBData(asbRecord, buffer, (editorState, engineState))
       |> then_(((allGameObjectsArr, (editorState, engineState))) => {
            editorState |> StateEditorService.setState |> ignore;
            engineState |> StateEngineService.setState |> ignore;

            (allGameObjectsArr, (basicMaterialMap, lightMaterialMap))
            |> resolve;
          })
       |> WonderBsMost.Most.fromPromise
     );
  /* |> then_(((imageMap, editorState)) => {
          let (textureMap, (editorState, engineState)) =
            _buildTextureData(asbRecord, imageMap, (editorState, engineState));
          let (basicMaterialMap, lightMaterialMap, (editorState, engineState)) =
            _buildMaterialData(
              asbRecord,
              textureMap,
              (editorState, engineState),
            );

          _buildWDBData(asbRecord, buffer, (editorState, engineState));
        })
     |> WonderBsMost.Most.fromPromise; */
};
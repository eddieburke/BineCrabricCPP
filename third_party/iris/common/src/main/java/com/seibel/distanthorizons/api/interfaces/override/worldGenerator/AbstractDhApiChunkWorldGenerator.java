/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.worldGenerator;

import com.seibel.distanthorizons.api.enums.EDhApiDetailLevel;
import com.seibel.distanthorizons.api.enums.worldGeneration.EDhApiDistantGeneratorMode;
import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGenerator;
import com.seibel.distanthorizons.api.objects.data.DhApiChunk;
import com.seibel.distanthorizons.coreapi.util.BitShiftUtil;
import java.io.Closeable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.function.Consumer;

public abstract class AbstractDhApiChunkWorldGenerator
implements Closeable,
IDhApiOverrideable,
IDhApiWorldGenerator {
    @Override
    public final byte getSmallestDataDetailLevel() {
        return EDhApiDetailLevel.BLOCK.detailLevel;
    }

    @Override
    public final byte getLargestDataDetailLevel() {
        return EDhApiDetailLevel.BLOCK.detailLevel;
    }

    @Override
    public final byte getMinGenerationGranularity() {
        return EDhApiDetailLevel.CHUNK.detailLevel;
    }

    @Override
    public final byte getMaxGenerationGranularity() {
        return (byte)(EDhApiDetailLevel.CHUNK.detailLevel + 2);
    }

    @Override
    public final CompletableFuture<Void> generateChunks(int chunkPosMinX, int chunkPosMinZ, byte granularity, byte targetDataDetail, EDhApiDistantGeneratorMode generatorMode, ExecutorService worldGeneratorThreadPool, Consumer<Object[]> resultConsumer) throws ClassCastException {
        return CompletableFuture.runAsync(() -> {
            int genChunkWidth = BitShiftUtil.powerOfTwo(granularity - 4);
            for (int chunkX = chunkPosMinX; chunkX < chunkPosMinX + genChunkWidth; ++chunkX) {
                for (int chunkZ = chunkPosMinZ; chunkZ < chunkPosMinZ + genChunkWidth; ++chunkZ) {
                    Object[] rawMcObjectArray = this.generateChunk(chunkX, chunkZ, generatorMode);
                    resultConsumer.accept(rawMcObjectArray);
                }
            }
        }, worldGeneratorThreadPool);
    }

    @Override
    public final CompletableFuture<Void> generateApiChunks(int chunkPosMinX, int chunkPosMinZ, byte granularity, byte targetDataDetail, EDhApiDistantGeneratorMode generatorMode, ExecutorService worldGeneratorThreadPool, Consumer<DhApiChunk> resultConsumer) {
        return CompletableFuture.runAsync(() -> {
            int genChunkWidth = BitShiftUtil.powerOfTwo(granularity - 4);
            for (int chunkX = chunkPosMinX; chunkX < chunkPosMinX + genChunkWidth; ++chunkX) {
                for (int chunkZ = chunkPosMinZ; chunkZ < chunkPosMinZ + genChunkWidth; ++chunkZ) {
                    DhApiChunk apiChunk = this.generateApiChunk(chunkX, chunkZ, generatorMode);
                    resultConsumer.accept(apiChunk);
                }
            }
        }, worldGeneratorThreadPool);
    }

    public abstract Object[] generateChunk(int var1, int var2, EDhApiDistantGeneratorMode var3);

    public abstract DhApiChunk generateApiChunk(int var1, int var2, EDhApiDistantGeneratorMode var3);
}

